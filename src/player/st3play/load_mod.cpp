#include "player/st3play/st3play.h"
#include <cmath>
#include "format/format.h"

namespace {

constexpr uint32_t MAGIC_M_K_ = MAGIC4('M', '.', 'K', '.');
constexpr uint32_t MAGIC_6CHN = MAGIC4('6', 'C', 'H', 'N');
constexpr uint32_t MAGIC_8CHN = MAGIC4('8', 'C', 'H', 'N');

constexpr double PERIOD_BASE  = 13696.0;  // C0 period

const uint16_t finetune_table[16] = {
    8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757,
    7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280
};

uint8_t period_to_note(uint16_t period)
{
    if (period == 0) {
        return 0;
    }

    return uint8_t(std::round(12.0 * std::log2(PERIOD_BASE / double(period))));
}

uint16_t convert_cmd(const uint8_t cmd, const uint8_t info)
{
    uint8_t new_cmd;
    uint8_t new_info = info;
    uint8_t x;

    switch (cmd) {
    case 0:         // Normal play or Arpeggio
        x = info ? 'J' : '@';
        break;
    case 1:         // Slide Up
        x = 'F';
        break;
    case 2:         // Slide Down
        x = 'E';
        break;
    case 3:         // Tone Portamento
        x = 'G';
        break;
    case 4:         // Vibrato
        x = 'H';
        break;
    case 5:         // Tone Portamento + Volume Slide
        x = 'L';
        break;
    case 6:         // Vibrato + Volume Slide
        x = 'K';
        break;
    case 7:         // Tremolo
        x = 'R';
        break;
    case 9:         // Set SampleOffset
        x = 'O';
        break;
    case 10:        // VolumeSlide
        x = 'D';
        break;
    case 11:        // Position Jump
        x = 'B';
        break;
    case 12:        // Set Volume
        new_info = 0;   // already set as volume
        x = '@';
        break;
    case 13:        // Pattern Break
        new_info = (info >> 4) * 10 + (info & 0x0f);
        x = 'C';
        break;
    case 14:        // E-Commands
        switch (info >> 4) {
        case 0:         // Set Filter
            x = 'S';
            break;
        case 1:         // E1- FineSlide Up
            new_info = 0xf0 | (info & 0x0f);
            x = 'F';
            break;
        case 2:         // E2- FineSlide Down
            new_info = 0xf0 | (info & 0x0f);
            x = 'E';
            break;
        case 3:         // E3- Glissando Control
            new_info = 0x10 | (info & 0x0f);
            x = 'S';
            break;
        case 4:         // E4- Set Vibrato Waveform
            new_info = 0x30 | (info & 0x0f);
            x = 'S';
            break;
        case 5:         // E5- Set Loop
            new_info = 0xb0;
            x = 'S';
            break;
        case 6:         // E6- Jump to Loop
            new_info = 0xb0 | (info & 0x0f);
            x = 'S';
            break;
        case 7:         // E7- Set Tremolo Waveform
            new_info = 0x40 | (info & 0x0f);
            x = 'S';
            break;
        case 9:         // E9- Retrig Note
            new_info = info & 0x0f;
            x = 'O';
            break;
        case 10:        // EA- Fine VolumeSlide Up
            new_info = ((info & 0x0f) << 4) | 0x0f;
            x = 'D';
            break;
        case 11:        // EB- Fine VolumeSlide Down
            new_info = 0xf0 | (info & 0x0f);
            x = 'D';
            break;
        case 12:        // EC- NoteCut
            x = 'S';
            break;
        case 13:        // ED- NoteDelay
            x = 'S';
            break;
        case 14:        // EE- PatternDelay
            x = 'S';
            break;
        case 15:        // EF- Invert Loop
            x = 'S';
            break;
        default:
            new_info = 0;
            x = '@';
        }
        break;
    case 15:        // Set Speed
        x = (info < 0x20) ? 'A' : 'T';
        break;
    default:
        x = '@';
    }

    new_cmd = x - '@';

    return (new_cmd << 8) | new_info;
}

uint8_t *encode_pattern(DataBuffer d, const int pat, const int num_chn)
{
    int size = 2;
    std::vector<uint8_t> data;

    data.push_back(0);   // make room for pattern size
    data.push_back(0);

    for (int r = 0; r < 64; r++) {
        for (int c = 0; c < num_chn; c++) {
            const uint32_t event = d.read32b(1084 + pat * 1024 + r * 16 + c * 4);
            uint16_t note = event >> 16;
            uint8_t cmd = (event & 0xff00) >> 8;
            uint8_t cmdlo = event & 0xff;

            uint8_t b = 0;
            if (note) {
                b |= 0x20;  // note and instrument follow
            }
            if ((cmd & 0x0f) == 0x0c) {
                b |= 0x40;  // volume follows
            }
            if (((cmd & 0x0f) || cmdlo) && (cmd & 0x0f) != 0x0c) {
                b |= 0x80;  // command and info follow
            }
            if (b) {
                b |= uint8_t(c);     // channel
                data.push_back(b); size++;
            }
            if (b & 0x20) {
                uint8_t n = period_to_note(note & 0xfff);
                n = (n == 0) ? 255 : (((n / 12) - 1) << 4) | (n % 12);  // hi=oct, lo=note, 255=empty note
                uint8_t ins = ((note & 0xf000) >> 8) | ((cmd & 0xf0) >> 4);
                data.push_back(n); size++;
                data.push_back(ins); size++;
            }
            if (b & 0x40) {
                data.push_back(cmdlo); size++;
            }
            if (b & 0x80) {
                uint16_t cmdinfo = convert_cmd(cmd & 0x0f, cmdlo);
                data.push_back(cmdinfo >> 8); size++;
                data.push_back(cmdinfo & 0xff); size++;
            }
        }
        data.push_back(0); size++;
    }

    uint8_t *buffer = new uint8_t[size];
    std::copy(data.begin(), data.end(), buffer);
    buffer[0] = size & 0xff;
    buffer[1] = size >> 8;

    return buffer;
}

}  // namespace


void St3Play::load_mod(DataBuffer const& d, int sr, SoftMixer *mixer)
{
    //const uint32_t modLen = d.size();

    audioRate = sr;
    mixer_ = mixer;

    memcpy(songname, d.ptr(0), 20);
    songname[20] = '\0';

    const uint32_t magic = d.read32b(1080);
    const int num_chn = (magic == MAGIC_8CHN) ? 8 : (magic == MAGIC_6CHN) ? 6 : 4;

    ordNum = d.read8(950); if (ordNum > 128) ordNum = 128;
    insNum = 31;
    patNum = 0;
    for (int i = 0; i < 128; i++) {
	uint16_t pat = d.read8(952 + i);
	patNum = std::max(patNum, pat);
    }
    patNum++;

    memcpy(order, d.ptr(952), ordNum);
    memset(chnsettings, 255, 32);
    for (int i = 0; i < num_chn; i++) {
        chnsettings[i] = (((i + 1) / 2) % 2) * 8;
    }

    // load instrument headers
    memset(ins, 0, sizeof (ins));
    for (int i = 0; i < insNum; ++i) {
	uint32_t offs = 20 + 30 * i;

	ins[i].type    = 0;
	ins[i].length  = d.read16b(offs + 22) * 2;
	ins[i].loopbeg = d.read16b(offs + 26) * 2;
	ins[i].looplen = d.read16b(offs + 28) * 2;
	ins[i].vol     = CLAMP(d.read8(offs + 25), 0, 63);
	ins[i].flags   = 0;
	ins[i].c2spd   = finetune_table[d.read8(offs + 24) & 0x0f];

	// reduce sample length if it overflows the module size
	//offs = ((d.read8(offs + 0xd) << 16) | d.read16l(offs + 0x0e)) << 4;
	//if ((offs + ins[i].length) >= modLen) {
	//    ins[i].length = modLen - offs;
	//}

	uint32_t loopEnd = ins[i].loopbeg + ins[i].looplen;

	if ((ins[i].loopbeg >= ins[i].length) || (loopEnd > ins[i].length)) {
	    ins[i].flags &= 0xFE;  // turn off loop
	}

	ins[i].looplen = loopEnd - ins[i].loopbeg;
	if ((ins[i].looplen == 0) || ((ins[i].loopbeg + ins[i].looplen) > ins[i].length)) {
	    ins[i].flags &= 0xFE;  // turn off loop
	}
    }

    // load pattern data
    memset(patdata, 0, sizeof (patdata));
    for (int i = 0; i < patNum; ++i) {
	patdata[i] = encode_pattern(d, i, num_chn);
    }
    patterns_in_place = false;

    // load sample data
    for (int i = 0; i < insNum; ++i) {
        uint32_t offs = 1084 + 1024 * patNum;
        ins[i].data = reinterpret_cast<int8_t *>(d.ptr(offs));
        offs += ins[i].length;

        mixer_->add_sample(ins[i].data, ins[i].length, 1.0);
    }
    instruments_in_place = true;

    // set up pans
    for (int i = 0; i < 32; ++i) {
        chn[i].apanpos = 0x7;
        if (chnsettings[i] != 0xFF) {
            chn[i].apanpos = (chnsettings[i] & 8) ? 0xC : 0x3;
        }
    }

    // count real amount of orders
    int k;
    for (k = (ordNum - 1); k >= 0; --k) {
        if (d.read8(0x60 + k) != 255) {
            break;
        }
    }
    ordNum = k;

    setspeed(6);
    settempo(125);
    setglobalvol(64);

    amigalimits  = true;
    oldstvib     = false;
    mastermul    = 0xb0;
    fastvolslide = false;

    stereomode = mastermul & 128;

    mastermul &= 127;
    if (mastermul == 0) {
        mastermul = 48;  // default in ST3 when you play a song where mastermul=0
    }

    aspdmin =  907 / 2;
    aspdmax = 1712 * 2;

    for (int i = 0; i < 32; ++i) {
        chn[i].channelnum   = (int8_t)(i);
        chn[i].achannelused = 0x80;
    }

    np_patseg    = NULL;
    musiccount   = 0;
    patterndelay = 0;
    patloopcount = 0;
    startrow     = 0;
    breakpat     = 0;
    volslidetype = 0;
    np_patoff    = -1;
    jmptoord     = -1;

    np_ord = 0;
    neworder();

    lastachannelused = 1;
}
