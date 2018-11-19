#include "player/pt21a.h"

// PT2.1A Replayer
//
// Based on the Protracker V2.1A replayer in m68k assembly language written
// by Peter "CRAYON" Hanning / Mushroom Studios in 1992. Original label and
// variable names used whenever possible.
//
// Bug fixes backported from Protracker 2.3D:
// * Mask finetune when playing voice
// * Mask note value in note delay command processing
// * Fix period table lookup by adding trailing zero values

constexpr int InitialSpeed = 6;
constexpr double InitialTempo = 125.0;


haze::Player *Protracker2::new_player(void *buf, uint32_t size, int sr)
{
    return new PT21A_Player(buf, size, sr);
}

//----------------------------------------------------------------------

PT21A_Player::PT21A_Player(void *buf, uint32_t size, int sr) :
    Player(buf, size, 4, sr),
    mt_SampleStarts{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    //mt_SongDataPtr(0),
    mt_speed(InitialSpeed),
    mt_counter(mt_speed),
    mt_SongPos(0),
    mt_PBreakPos(0),
    mt_PosJumpFlag(false),
    mt_PBreakFlag(false),
    mt_LowMask(0),
    mt_PattDelTime(0),
    mt_PattDelTime2(0),
    mt_PatternPos(0),
    cia_tempo(InitialTempo)
{
    memset(mt_chantemp, 0, sizeof(mt_chantemp));
}

PT21A_Player::~PT21A_Player()
{
}

void PT21A_Player::start()
{
    speed_ = InitialSpeed;
    tempo_ = InitialTempo;
    time_ = 0.0f;

    initial_speed_ = speed_;
    initial_tempo_ = tempo_;

    int num_pat = 0;
    for (int i = 0; i < 128; i++) {
        int pat = mdata.read8(952 + i);
        num_pat = std::max(num_pat, pat);
    }
    num_pat++;

    int offset = 1084 + 1024 * num_pat;
    for (int i = 0; i < 31; i++) {
        mt_SampleStarts[i] = offset;
        offset += mdata.read16b(20 + 22 + 30 * i) * 2;
    }

    mixer_->add_sample(mdata.ptr(0), mdata.size());
    mixer_->set_sample(0, 0);
    mixer_->set_sample(1, 0);
    mixer_->set_sample(2, 0);
    mixer_->set_sample(3, 0);

    const int pan = options.get("pan", 70);
    const int panl = -128 * pan / 100;
    const int panr = 127 * pan / 100;

    mixer_->set_pan(0, panl);
    mixer_->set_pan(1, panr);
    mixer_->set_pan(2, panr);
    mixer_->set_pan(3, panl);

}

void PT21A_Player::play()
{
    mt_music();

    tempo_ = cia_tempo;
    time_ += 20.0 * 125.0 / tempo_;
}

void PT21A_Player::reset()
{
}

void PT21A_Player::frame_info(haze::FrameInfo& pi)
{
    pi.pos = mt_SongPos;
    pi.row = mt_PatternPos;
    pi.num_rows = 64;
    pi.frame = mt_speed - mt_counter;
    pi.song = 0;
    pi.speed = mt_speed;
    pi.tempo = cia_tempo;
    pi.time = time_;
}

//----------------------------------------------------------------------

namespace {

const uint8_t mt_VibratoTable[32] = {
      0,  24,  49,  74,  97, 120, 141, 161,
    180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197,
    180, 161, 141, 120,  97,  74,  49,  24
};

// PT2.3D fix: add trailing zeros
const uint16_t mt_PeriodTable[16*37] = {
// Tuning 0, Normal
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113, 0,
// Tuning 1
    850, 802, 757, 715, 674, 637, 601, 567, 535, 505, 477, 450,
    425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 239, 225,
    213, 201, 189, 179, 169, 159, 150, 142, 134, 126, 119, 113, 0,
// Tuning 2
    844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474, 447,
    422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237, 224,
    211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 112, 0,
// Tuning 3
    838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470, 444,
    419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235, 222,
    209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 111, 0,
// Tuning 4
    832, 785, 741, 699, 660, 623, 588, 555, 524, 495, 467, 441,
    416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233, 220,
    208, 196, 185, 175, 165, 156, 147, 139, 131, 124, 117, 110, 0,
// Tuning 5
    826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463, 437,
    413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232, 219,
    206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 109, 0,
// Tuning 6
    820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460, 434,
    410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230, 217,
    205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 109, 0,
// Tuning 7
    814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457, 431,
    407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228, 216,
    204, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 108, 0,
// Tuning -8
    907, 856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480,
    453, 428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240,
    226, 214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 0,
// Tuning -7
    900, 850, 802, 757, 715, 675, 636, 601, 567, 535, 505, 477,
    450, 425, 401, 379, 357, 337, 318, 300, 284, 268, 253, 238,
    225, 212, 200, 189, 179, 169, 159, 150, 142, 134, 126, 119, 0,
// Tuning -6
    894, 844, 796, 752, 709, 670, 632, 597, 563, 532, 502, 474,
    447, 422, 398, 376, 355, 335, 316, 298, 282, 266, 251, 237,
    223, 211, 199, 188, 177, 167, 158, 149, 141, 133, 125, 118, 0,
// Tuning -5
    887, 838, 791, 746, 704, 665, 628, 592, 559, 528, 498, 470,
    444, 419, 395, 373, 352, 332, 314, 296, 280, 264, 249, 235,
    222, 209, 198, 187, 176, 166, 157, 148, 140, 132, 125, 118, 0,
// Tuning -4
    881, 832, 785, 741, 699, 660, 623, 588, 555, 524, 494, 467,
    441, 416, 392, 370, 350, 330, 312, 294, 278, 262, 247, 233,
    220, 208, 196, 185, 175, 165, 156, 147, 139, 131, 123, 117, 0,
// Tuning -3
    875, 826, 779, 736, 694, 655, 619, 584, 551, 520, 491, 463,
    437, 413, 390, 368, 347, 328, 309, 292, 276, 260, 245, 232,
    219, 206, 195, 184, 174, 164, 155, 146, 138, 130, 123, 116, 0,
// Tuning -2
    868, 820, 774, 730, 689, 651, 614, 580, 547, 516, 487, 460,
    434, 410, 387, 365, 345, 325, 307, 290, 274, 258, 244, 230,
    217, 205, 193, 183, 172, 163, 154, 145, 137, 129, 122, 115, 0,
// Tuning -1
    862, 814, 768, 725, 684, 646, 610, 575, 543, 513, 484, 457,
    431, 407, 384, 363, 342, 323, 305, 288, 272, 256, 242, 228,
    216, 203, 192, 181, 171, 161, 152, 144, 136, 128, 121, 114, 0
};

}  // namespace


void PT21A_Player::mt_music()
{
    mt_counter++;
    if (mt_speed > mt_counter) {
        // mt_NoNewNote
        mt_NoNewAllChannels();
        mt_NoNewPosYet();
        return;
    }

    mt_counter = 0;
    if (mt_PattDelTime2 == 0) {
        mt_GetNewNote();
    } else {
        mt_NoNewAllChannels();
    }

    // mt_dskip
    mt_PatternPos++;
    if (mt_PattDelTime) {
        mt_PattDelTime2 = mt_PattDelTime;
        mt_PattDelTime = 0;
    }

    // mt_dskc
    if (mt_PattDelTime2) {
        mt_PattDelTime2--;
        if (mt_PattDelTime2) {
            mt_PatternPos -= 1;
        }
    }

    // mt_dska
    if (mt_PBreakFlag) {
        mt_PBreakFlag = false;
        mt_PatternPos = mt_PBreakPos;
        mt_PBreakPos = 0;
    }

    // mt_nnpysk
    if (mt_PatternPos >= 64) {
        mt_NextPosition();
    }
    mt_NoNewPosYet();
}

void PT21A_Player::mt_NoNewAllChannels()
{
    mt_CheckEfx(0);
    mt_CheckEfx(1);
    mt_CheckEfx(2);
    mt_CheckEfx(3);
}

void PT21A_Player::mt_GetNewNote()
{
    const int pat = mdata.read8(952 + mt_SongPos);

    mt_PlayVoice(pat, 0);
    mt_PlayVoice(pat, 1);
    mt_PlayVoice(pat, 2);
    mt_PlayVoice(pat, 3);

    // mt_SetDMA
    for (int chn = 0; chn < 4; chn++) {
        auto& ch = mt_chantemp[chn];
        mixer_->set_loop_start(chn, ch.n_loopstart);
        mixer_->set_loop_end(chn, ch.n_loopstart + ch.n_replen * 2);
    }
}

void PT21A_Player::mt_PlayVoice(const int pat, const int chn)
{
    const uint32_t event = mdata.read32b(1084 + pat * 1024 + mt_PatternPos * 16 + chn * 4);
    if (event == 0) {   // TST.L   (A6)
        mt_PerNop(chn);
        return;
    }

    // mt_plvskip
    auto& ch = mt_chantemp[chn];

    ch.n_note = event >> 16;      // MOVE.L  (A0,D1.L),(A6)
    ch.n_cmd = (event & 0xff00) >> 8;
    ch.n_cmdlo = event & 0xff;

    const int ins = ((ch.n_note & 0xf000) >> 8) | ((ch.n_cmd & 0xf0) >> 4);

    if (ins > 0 && ins <= 31) {       // sanity check: was: ins != 0
        int ofs = 20 + 30 * (ins - 1) + 22;
        ch.n_start = mt_SampleStarts[ins - 1];
        ch.n_length = mdata.read16b(ofs);
        ch.n_reallength = ch.n_length;
        // PT2.3D fix: mask finetune
        ch.n_finetune = mdata.read8(ofs + 2) & 0x0f;
        ch.n_volume = mdata.read8(ofs + 3);

        const uint32_t repeat = mdata.read16b(ofs + 4);

        if (repeat) {                             // TST.W   D3 / BEQ.S   mt_NoLoop
            ch.n_loopstart = ch.n_start + repeat * 2;
            ch.n_wavestart = ch.n_loopstart;
            ch.n_length = repeat + mdata.read16b(ofs + 6);
            ch.n_replen = mdata.read16b(ofs + 6);       // MOVE.W  6(A3,D4.L),n_replen(A6) ; Save replen
            mixer_->set_volume(chn, ch.n_volume << 2);  // MOVE.W  D0,8(A5)        ; Set volume
        } else {
            // mt_NoLoop
            ch.n_loopstart = ch.n_start;
            ch.n_wavestart = ch.n_start;
            ch.n_replen = mdata.read16b(ofs + 6);       // MOVE.W  6(A3,D4.L),n_replen(A6) ; Save replen
            mixer_->set_volume(chn, ch.n_volume << 2);  // MOVE.W  D0,8(A5)        ; Set volume
        }
        mixer_->enable_loop(chn, repeat != 0);

    }

    // mt_SetRegs
    if (ch.n_note & 0xfff) {
        switch (ch.n_cmd & 0x0f) {
        case 0xe:
            if ((ch.n_cmdlo & 0xf0) == 0x50) {
                // mt_DoSetFinetune
                mt_SetFinetune(chn);
            }
            mt_SetPeriod(chn);
            break;
        case 0x3:  // TonePortamento
        case 0x5:
            mt_SetTonePorta(chn);
            mt_CheckMoreEfx(chn);
            break;
        case 0x9:  // Sample Offset
            mt_CheckMoreEfx(chn);
            mt_SetPeriod(chn);
            break;
        default:
            mt_SetPeriod(chn);
        }
    } else {
        mt_CheckMoreEfx(chn);  // If no note
    }
}

void PT21A_Player::mt_SetPeriod(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int note = ch.n_note & 0xfff;

    int i = 0;                              // MOVEQ   #0,D0
    // mt_ftuloop
    while (i < 36) {
        if (note >= mt_PeriodTable[i]) {    // CMP.W   (A1,D0.W),D1
            break;                          // BHS.S   mt_ftufound
        }
        i++;                                // ADDQ.L  #2,D0
    }                                       // DBRA    D7,mt_ftuloop
    // mt_ftufound
    ch.n_period = mt_PeriodTable[37 * ch.n_finetune + i];

    if ((ch.n_cmd & 0x0f) != 0x0e || (ch.n_cmdlo & 0xf0) != 0xd0) {  // !Notedelay
        if ((ch.n_wavecontrol & 0x04) != 0x00) {
            ch.n_vibratopos = 0;
        }
        // mt_vibnoc
        if ((ch.n_wavecontrol & 0x40) != 0x00) {
            ch.n_tremolopos = 0;
        }
        // mt_trenoc
        mixer_->set_start(chn, ch.n_start);
        mixer_->set_end(chn, ch.n_start + ch.n_length * 2);
        mixer_->set_period(chn, ch.n_period);
    }

    mt_CheckMoreEfx(chn);
}

void PT21A_Player::mt_NextPosition()
{
    mt_PatternPos = mt_PBreakPos;
    mt_PBreakPos = 0;
    mt_PosJumpFlag = false;
    mt_SongPos = ++mt_SongPos;
    mt_SongPos &= 0x7f;
    if (mt_SongPos >= mdata.read8(950)) {
        mt_SongPos = 0;
    }
}

void PT21A_Player::mt_NoNewPosYet()
{
    if (mt_PosJumpFlag) {
        mt_NextPosition();
        mt_NoNewPosYet();
    }
}

void PT21A_Player::mt_CheckEfx(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int cmd = ch.n_cmd & 0x0f;

    mt_UpdateFunk(chn);

    if (cmd == 0 && ch.n_cmdlo == 0) {
        mt_PerNop(chn);
        return;
    }

    switch (cmd) {
    case 0x0:
        mt_Arpeggio(chn);
        break;
    case 0x1:
        mt_PortaUp(chn);
        break;
    case 0x2:
        mt_PortaDown(chn);
        break;
    case 0x3:
        mt_TonePortamento(chn);
        break;
    case 0x4:
        mt_Vibrato(chn);
        break;
    case 0x5:
        mt_TonePlusVolSlide(chn);
        break;
    case 0x6:
        mt_VibratoPlusVolSlide(chn);
        break;
    case 0xe:
        mt_E_Commands(chn);
        break;
    default:
        // SetBack
        mixer_->set_period(chn, ch.n_period);  // MOVE.W  n_period(A6),6(A5)
        switch (cmd) {
        case 0x7:
            mt_Tremolo(chn);
            break;
        case 0xa:
            mt_VolumeSlide(chn);
            break;
        }
    }

    if (cmd != 0x7) {
        mixer_->set_volume(chn, ch.n_volume << 2);
    }
}

void PT21A_Player::mt_PerNop(const int chn)
{
    const int period = mt_chantemp[chn].n_period;
    mixer_->set_period(chn, period);  // MOVE.W  n_period(A6),6(A5)
}


void PT21A_Player::mt_Arpeggio(const int chn)
{
    auto& ch = mt_chantemp[chn];

    int val;
    switch (mt_counter % 3) {
    case 2:  // Arpeggio1
        val = ch.n_cmdlo & 15;
        break;
    case 0:  // Arpeggio2
        val = 0;
        break;
    default:
        val = ch.n_cmdlo >> 4;
    }

    // Arpeggio3
    const int ofs = 37 * ch.n_finetune;  // MOVE.B  n_finetune(A6),D1 / MULU    #36*2,D1

    // mt_arploop
    for (int i = 0; i < 36; i++) {
	if (ch.n_period >= mt_PeriodTable[ofs + i]) {
	   // Arpeggio4
	   mixer_->set_period(chn, mt_PeriodTable[ofs + i + val]);  // MOVE.W  D2,6(A5)
	   return;
	}
    }
}

void PT21A_Player::mt_FinePortaUp(const int chn)
{
    if (mt_counter) {
	return;
    }
    mt_LowMask = 0x0f;
    mt_PortaUp(chn);
}

void PT21A_Player::mt_PortaUp(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_period -= (ch.n_cmdlo & mt_LowMask);
    mt_LowMask = 0xff;
    if (ch.n_period < 113) {
	ch.n_period = 113;
    }
    mixer_->set_period(chn, ch.n_period);  // MOVE.W  n_period(A6),6(A5)
}

void PT21A_Player::mt_FinePortaDown(const int chn)
{
    if (mt_counter) {
	return;
    }
    mt_LowMask = 0x0f;
    mt_PortaDown(chn);
}

void PT21A_Player::mt_PortaDown(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_period += ch.n_cmdlo & mt_LowMask;
    mt_LowMask = 0xff;
    if (ch.n_period > 856) {
	ch.n_period = 856;
    }
    mixer_->set_period(chn, ch.n_period);  // MOVE.W  D0,6(A5)
}

void PT21A_Player::mt_SetTonePorta(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int note = ch.n_note & 0xfff;
    const int ofs = 37 * ch.n_finetune;  // MOVE.B  n_finetune(A6),D0 / MULU    #37*2,D0

    int i = 0;           // MOVEQ   #0,D0
    // mt_StpLoop
    while (note < mt_PeriodTable[ofs + i]) {    // BHS.S   mt_StpFound
	i++;             // ADDQ.W  #2,D0
	if (i >= 37) {   // CMP.W   #37*2,D0 / BLO.S   mt_StpLoop
	    i = 35;      // MOVEQ   #35*2,D0
	    break;
	}
    }

    // mt_StpFound
    if ((ch.n_finetune & 0x80) != 0 && i) {
	i--;             // SUBQ.W  #2,D0
    }
    // mt_StpGoss
    ch.n_wantedperiod = mt_PeriodTable[ofs + i];
    ch.n_toneportdirec = false;

    if (ch.n_period == ch.n_wantedperiod) {
	// mt_ClearTonePorta
	ch.n_wantedperiod = 0;
    } else if (ch.n_period < ch.n_wantedperiod) {
	ch.n_toneportdirec = true;
    }
}

void PT21A_Player::mt_TonePortamento(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo) {
        ch.n_toneportspeed = ch.n_cmdlo;
        ch.n_cmdlo = 0;
    }
    mt_TonePortNoChange(chn);
}

void PT21A_Player::mt_TonePortNoChange(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_wantedperiod == 0) {
	return;
    }
    if (ch.n_toneportdirec) {
	// mt_TonePortaDown
	ch.n_period += ch.n_toneportspeed;
	if (ch.n_period > ch.n_wantedperiod) {
	    ch.n_period = ch.n_wantedperiod;
	    ch.n_wantedperiod = 0;
	}
    } else {
	// mt_TonePortaUp
	if (ch.n_period > ch.n_toneportspeed) {
	    ch.n_period -= ch.n_toneportspeed;
	} else {
	    ch.n_period = 0;
	}
	if (ch.n_period < ch.n_wantedperiod) {
	    ch.n_period = ch.n_wantedperiod;
	    ch.n_wantedperiod = 0;
	}
    }
    // mt_TonePortaSetPer
    int period = ch.n_period;                        // MOVE.W  n_period(A6),D2
    if (ch.n_glissfunk & 0x0f) {
	int ofs = 37 * ch.n_finetune;                // MULU    #36*2,D0
	int i = 0;
	// mt_GlissLoop
	while (period < mt_PeriodTable[ofs + i]) {   // LEA     mt_PeriodTable(PC),A0 / CMP.W   (A0,D0.W),D2
	    i++;
	    if (i >= 37) {
		i = 35;
		break;
	    }
	}
	// mt_GlissFound
	period = mt_PeriodTable[ofs + i];           // MOVE.W  (A0,D0.W),D2
    }
    // mt_GlissSkip
    mixer_->set_period(chn, period);                  // MOVE.W  D2,6(A5) ; Set period
}

void PT21A_Player::mt_Vibrato(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo) {
        if (ch.n_cmdlo & 0x0f) {
            ch.n_vibratocmd = (ch.n_vibratocmd & 0xf0) | (ch.n_cmdlo & 0x0f);
        }
        // mt_vibskip
        if (ch.n_cmdlo & 0xf0) {
            ch.n_vibratocmd = (ch.n_vibratocmd & 0x0f) | (ch.n_cmdlo & 0xf0);
        }
        // mt_vibskip2
    }
    mt_Vibrato2(chn);
}

void PT21A_Player::mt_Vibrato2(const int chn)
{
    auto& ch = mt_chantemp[chn];
    int pos = (ch.n_vibratopos >> 2) & 0x1f;

    int val = 255;
    switch (ch.n_wavecontrol & 0x03) {
    case 0:  // mt_vib_sine
        val = mt_VibratoTable[pos];
        break;
    case 1:  // mt_vib_rampdown
        pos <<= 3;
        val = (pos & 0x80) ? 255 - pos : pos;
        break;
    }

    // mt_vib_set
    int period = ch.n_period;
    const int amt = (val * (ch.n_vibratocmd & 15)) >> 7;
    if ((ch.n_vibratopos & 0x80) == 0) {
	period += amt;
    } else {
	period -= amt;
    }

    // mt_Vibrato3
    mixer_->set_period(chn, period);
    ch.n_vibratopos += (ch.n_vibratocmd >> 2) & 0x3c;
}

void PT21A_Player::mt_TonePlusVolSlide(const int chn)
{
    mt_TonePortNoChange(chn);
    mt_VolumeSlide(chn);
}

void PT21A_Player::mt_VibratoPlusVolSlide(const int chn)
{
    mt_Vibrato2(chn);
    mt_VolumeSlide(chn);
}

void PT21A_Player::mt_Tremolo(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo) {
	if (ch.n_cmdlo & 0x0f) {
	     ch.n_tremolocmd = (ch.n_cmdlo & 0x0f) | (ch.n_tremolocmd & 0xf0);
	}
	// mt_treskip
	if (ch.n_cmdlo & 0xf0) {
             ch.n_tremolocmd = (ch.n_cmdlo & 0xf0) | (ch.n_tremolocmd & 0x0f);
        }
        // mt_treskip2
    }
    // mt_Tremolo2
    int pos = (ch.n_tremolopos >> 2) & 0x1f;

    int val = 255;
    switch ((ch.n_wavecontrol >> 4) & 0x03) {
    case 0:  // mt_tre_sine
        val = mt_VibratoTable[pos];
        break;
    case 1:  // mt_rampdown
        pos <<= 3;
        val = (pos & 0x80) ? 255 - pos : pos;
        break;
    }

    // mt_tre_set
    int volume = ch.n_volume;
    const int amt = (val * (ch.n_tremolocmd & 15)) >> 6;

    if ((ch.n_tremolopos & 0x80) == 0) {
        volume += amt;
    } else {
        volume -= amt;
    }

    // mt_Tremolo3
    if (volume < 0) {
        volume = 0;
    }

    // mt_TremoloSkip
    if (volume > 0x40) {
       volume = 0x40;
    }

    // mt_TremoloOk
    mixer_->set_volume(chn, volume << 2);  // MOVE.W  D0,8(A5)
    ch.n_tremolopos += (ch.n_tremolocmd >> 2) & 0x3c;
}

void PT21A_Player::mt_SampleOffset(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo) {
        ch.n_sampleoffset = ch.n_cmdlo;
    }
    // mt_sononew
    const uint16_t offset = ch.n_sampleoffset << 7;
    if (offset < ch.n_length) {
        ch.n_length -= offset;
        ch.n_start += offset << 1;
        return;
    }
    // mt_sofskip
    ch.n_length = 1;
}

void PT21A_Player::mt_VolumeSlide(const int chn)
{
    if ((mt_chantemp[chn].n_cmdlo & 0xf0) == 0) {
        mt_VolSlideDown(chn);
    } else {
        mt_VolSlideUp(chn);
    }
}

void PT21A_Player::mt_VolSlideUp(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_volume += ch.n_cmdlo >> 4;
    if (ch.n_volume > 0x40) {
        ch.n_volume = 0x40;
    }
    mixer_->set_volume(chn, ch.n_volume << 2);
}

void PT21A_Player::mt_VolSlideDown(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int val = ch.n_cmdlo & 0x0f;
    if (ch.n_volume > val) {
        ch.n_volume -= val;
    } else {
        ch.n_volume = 0;
    }
    mixer_->set_volume(chn, ch.n_volume << 2);
}

void PT21A_Player::mt_PositionJump(const int chn)
{
    auto& ch = mt_chantemp[chn];
    mt_SongPos = --ch.n_cmdlo;
    // mt_pj2
    mt_PBreakPos = 0;
    mt_PosJumpFlag = true;
}

void PT21A_Player::mt_VolumeChange(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo > 0x40) {
        ch.n_cmdlo = 0x40;
    }
    ch.n_volume = ch.n_cmdlo;
    mixer_->set_volume(chn, ch.n_volume << 2);  // MOVE.W  D0,8(A5)
}

void PT21A_Player::mt_PatternBreak(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int row = (ch.n_cmdlo >> 4) * 10 + (ch.n_cmdlo & 0x0f);
    if (row <= 63) {
        // mt_pj2
        mt_PBreakPos = row;
    }
    mt_PosJumpFlag = true;
}

void PT21A_Player::mt_SetSpeed(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (ch.n_cmdlo) {
        mt_counter = 0;
        // also check CIA tempo
        if (ch.n_cmdlo < 0x20) {
            mt_speed = ch.n_cmdlo;
        } else {
            cia_tempo = ch.n_cmdlo;
        }
    }
}

void PT21A_Player::mt_CheckMoreEfx(const int chn)
{
    mt_UpdateFunk(chn);

    switch (mt_chantemp[chn].n_cmd & 0x0f) {
    case 0x9:
        mt_SampleOffset(chn);
        break;
    case 0xb:
        mt_PositionJump(chn);
        break;
    case 0xd:
        mt_PatternBreak(chn);
        break;
    case 0xe:
        mt_E_Commands(chn);
        break;
    case 0xf:
        mt_SetSpeed(chn);
        break;
    case 0xc:
        mt_VolumeChange(chn);
        break;
    }

    mt_PerNop(chn);
}

void PT21A_Player::mt_E_Commands(const int chn)
{
    switch (mt_chantemp[chn].n_cmdlo >> 4) {
    case 0x0:
        mt_FilterOnOff(chn);
        break;
    case 0x1:
        mt_FinePortaUp(chn);
        break;
    case 0x2:
        mt_FinePortaDown(chn);
        break;
    case 0x3:
        mt_SetGlissControl(chn);
        break;
    case 0x4:
        mt_SetVibratoControl(chn);
        break;
    case 0x5:
        mt_SetFinetune(chn);
        break;
    case 0x6:
        mt_JumpLoop(chn);
        break;
    case 0x7:
        mt_SetTremoloControl(chn);
        break;
    case 0x9:
        mt_RetrigNote(chn);
        break;
    case 0xa:
        mt_VolumeFineUp(chn);
        break;
    case 0xb:
        mt_VolumeFineDown(chn);
        break;
    case 0xc:
        mt_NoteCut(chn);
        break;
    case 0xd:
        mt_NoteDelay(chn);
        break;
    case 0xe:
        mt_PatternDelay(chn);
        break;
    case 0xf:
        mt_FunkIt(chn);
        break;
    }
}

void PT21A_Player::mt_FilterOnOff(const int chn)
{
    auto& ch = mt_chantemp[chn];
    mixer_->enable_filter(ch.n_cmdlo & 0x0f);
}

void PT21A_Player::mt_SetGlissControl(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_glissfunk = (ch.n_glissfunk & 0xf0) | (ch.n_cmdlo & 0x0f);
}

void PT21A_Player::mt_SetVibratoControl(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_wavecontrol &= 0xf0;
    ch.n_wavecontrol |= ch.n_cmdlo & 0x0f;
}

void PT21A_Player::mt_SetFinetune(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_finetune = ch.n_cmdlo & 0x0f;
}

void PT21A_Player::mt_JumpLoop(const int chn)
{
    auto& ch = mt_chantemp[chn];

    if (mt_counter) {
        return;
    }

    const int cmdlo = ch.n_cmdlo & 0x0f;

    if (cmdlo == 0) {
        // mt_SetLoop
        ch.n_pattpos = mt_PatternPos;
    } else {
        if (ch.n_loopcount == 0) {
            // mt_jmpcnt
            ch.n_loopcount = cmdlo;
            ch.inside_loop = true;
        } else {
            ch.n_loopcount -= 1;
            if (ch.n_loopcount == 0) {
                ch.inside_loop = false;
                return;
            }
        }
        // mt_jmploop
        mt_PBreakPos = ch.n_pattpos;
        mt_PBreakFlag = true;
    }
}

void PT21A_Player::mt_SetTremoloControl(const int chn)
{
    auto& ch = mt_chantemp[chn];
    ch.n_wavecontrol &= 0x0f;
    ch.n_wavecontrol |= (ch.n_cmdlo & 0x0f) << 4;
}

void PT21A_Player::mt_RetrigNote(const int chn)
{
    auto& ch = mt_chantemp[chn];
    const int cmdlo = ch.n_cmdlo & 0x0f;
    if (cmdlo == 0) {
        return;
    }
    if (mt_counter == 0) {
        if (ch.n_note & 0xfff) {
            return;
        }
    }
    // mt_rtnskp
    if (mt_counter % cmdlo) {
        return;
    }

    // mt_DoRetrig
    mixer_->set_start(chn, ch.n_start);
    mixer_->set_end(chn, ch.n_start + ch.n_length * 2);
}

void PT21A_Player::mt_VolumeFineUp(const int chn)
{
    if (mt_counter) {
        return;
    }
    mt_VolSlideUp(chn);
}

void PT21A_Player::mt_VolumeFineDown(const int chn)
{
    if (mt_counter) {
        return;
    }
    mt_VolSlideDown(chn);
}

void PT21A_Player::mt_NoteCut(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (mt_counter != ch.n_cmdlo) {
        return;
    }
    ch.n_volume = 0;
    mixer_->set_volume(chn, 0);  // MOVE.W  #0,8(A5)
}

void PT21A_Player::mt_NoteDelay(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (mt_counter != ch.n_cmdlo) {
        return;
    }
    // PT2.3D fix: mask note
    if ((ch.n_note & 0xfff) == 0) {
        return;
    }
    // BRA mt_DoRetrig
    mixer_->set_start(chn, ch.n_start);
    mixer_->set_end(chn, ch.n_start + ch.n_length * 2);
}

void PT21A_Player::mt_PatternDelay(const int chn)
{
    auto& ch = mt_chantemp[chn];
    if (mt_counter) {
        return;
    }
    if (mt_PattDelTime2) {
        return;
    }
    mt_PattDelTime = (ch.n_cmdlo & 0x0f) + 1;
}

void PT21A_Player::mt_FunkIt(const int chn)
{
}

void PT21A_Player::mt_UpdateFunk(const int chn)
{
}

