#include "player/st3play/st3play.h"
#include "format/format.h"

namespace {

constexpr uint32_t MAGIC_M_K_ = MAGIC4('M', '.', 'K', '.');
constexpr uint32_t MAGIC_6CHN = MAGIC4('6', 'C', 'H', 'N');
constexpr uint32_t MAGIC_8CHN = MAGIC4('8', 'C', 'H', 'N');

}  // namespace

void St3Play::load_mod(DataBuffer const& d, int sr, SoftMixer *mixer)
{
    const uint32_t modLen = d.size();

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

    memcpy(order,       d.ptr(952), ordNum);
    //memcpy(chnsettings, d.ptr(0x40), 32);

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
        ins[i].c2spd   = 0; //d.read16l(offs + 0x20);  // ST3 only reads the lower word of this one

        // reduce sample length if it overflows the module size
        //offs = ((d.read8(offs + 0xd) << 16) | d.read16l(offs + 0x0e)) << 4;
        //if ((offs + ins[i].length) >= modLen) {
        //    ins[i].length = modLen - offs;
        //}

        int loopEnd = ins[i].loopbeg + ins[i].looplen;

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
        uint32_t offs = 1084 + 1024 * i;

        //uint16_t patDataLen = d.read16l(offs);
        //if (patDataLen > 0) {
        //    patdata[i] = d.ptr(offs + 2);
        //}
    }

    // load sample data
    for (int i = 0; i < insNum; ++i) {
        uint32_t offs = 1084 + 1024 * patNum;
        ins[i].data = reinterpret_cast<int8_t *>(d.ptr(offs));
        offs += ins[i].length;

        mixer_->add_sample(ins[i].data, ins[i].length, 1.0);
    }

    // set up pans
    for (int i = 0; i < 32; ++i) {
        chn[i].apanpos = 0x7;
        if (chnsettings[i] != 0xFF) {
            chn[i].apanpos = (chnsettings[i] & 8) ? 0xC : 0x3;
        }

        if ((soundcardtype == SOUNDCARD_GUS) && (d.read8(0x35) == 252)) {
            // non-default pannings
            uint8_t pan = d.read8(0x60 + ordNum + (insNum * 2) + (patNum * 2) + i);
            if (pan & 32) {
                chn[i].apanpos = pan & 0x0F;
            }
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

