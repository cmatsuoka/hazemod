#include "player/st3play/st3play.h"


namespace st3play {

void St3Play::load_s3m(DataBuffer const& d, int sr, SoftMixer *mixer)
{
    uint32_t modLen = d.size();

    audioRate = sr;
    mixer_ = mixer;

    memcpy(songname, d.ptr(0), 28);
    songname[28] = '\0';

    bool signedSamples = (d.read16l(0x2a) == 1);

    ordNum = d.read16l(0x20); if (ordNum > 256) ordNum = 256;
    insNum = d.read16l(0x22); if (insNum > 100) insNum = 100;
    patNum = d.read16l(0x24); if (patNum > 100) patNum = 100;

    memcpy(order,       d.ptr(0x60), ordNum);
    memcpy(chnsettings, d.ptr(0x40), 32);

    // load instrument headers
    memset(ins, 0, sizeof (ins));
    for (int i = 0; i < insNum; ++i) {
        uint32_t offs = (d.read16l(0x60 + ordNum + (i * 2))) << 4;
        if (offs == 0) {
            continue;  // empty
        }

        ins[i].type    = d.read8(offs);
        ins[i].length  = d.read32l(offs + 0x10);
        ins[i].loopbeg = d.read32l(offs + 0x14);
        uint32_t loopEnd = d.read32l(offs + 0x18);
        ins[i].vol     = CLAMP(d.read8i(offs + 0x1c), 0, 63);  // ST3 clamps smp vol to 63, to prevent mix vol overflow
        ins[i].flags   = d.read8(offs + 0x1f);
        ins[i].c2spd   = d.read16l(offs + 0x20);  // ST3 only reads the lower word of this one

        // reduce sample length if it overflows the module size (f.ex. "miracle man.s3m")
        offs = ((d.read8(offs + 0xd) << 16) | d.read16l(offs + 0x0e)) << 4;
        if ((offs + ins[i].length) >= modLen) {
            ins[i].length = modLen - offs;
        }

        if (loopEnd < ins[i].loopbeg) {
            loopEnd = ins[i].loopbeg + 1;
        }

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
        uint32_t offs = (d.read16l(0x60 + ordNum + (insNum * 2) + (i * 2))) << 4;
        if (offs == 0) {
            continue;  // empty
        }

        uint16_t patDataLen = d.read16l(offs);
        if (patDataLen > 0) {
            patdata[i] = d.ptr(offs + 2);
        }
    }
    patterns_in_place = true;

    // load sample data
    for (int i = 0; i < insNum; ++i) {
        uint32_t offs = (d.read16l(0x60 + ordNum + (i * 2))) << 4;
        if (offs == 0) {
            mixer_->add_sample(ins[i].data, 0);
            continue;  // empty
        }

        if ((ins[i].length <= 0) || (ins[i].type != 1) || (d.read8(offs + 0x1E) != 0)) {
            mixer_->add_sample(ins[i].data, 0);
            continue;  // sample not supported
        }

        offs = ((d.read8(offs + 0xd) << 16) | d.read16l(offs + 0x0e)) << 4;
        if (offs == 0) {
            mixer_->add_sample(ins[i].data, 0);
            continue;  // empty
        }

        // offs now points to sample data

        if (ins[i].flags & 4) {  // 16-bit
            ins[i].data = reinterpret_cast<int8_t *>(new int16_t[ins[i].length]);
        } else {
            ins[i].data = new int8_t[ins[i].length];
        }

        if (ins[i].flags & 4) {
            // 16-bit
            if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
                memcpy(ins[i].data, d.ptr(offs), ins[i].length * 2);
            } else {
                // copy converting to native endian
                int8_t *src = reinterpret_cast<int8_t *>(d.ptr(offs));
                int8_t *dst = ins[i].data;
                for (uint32_t j = 0; j < ins[i].length; ++j) {
                    *dst++ = src[1];
                    *dst++ = *src;
                    src += 2;
                }
            }
            if (!signedSamples) {
                int16_t *d = reinterpret_cast<int16_t *>(ins[i].data);
                for (uint32_t j = 0; j < ins[i].length; ++j) {
                    *d += 32768;
                }
            }
        } else {
            // 8-bit
            if (signedSamples) {
                memcpy(ins[i].data, d.ptr(offs), ins[i].length);
            } else {
                // copy converting to signed samples
                int8_t *src = reinterpret_cast<int8_t *>(d.ptr(offs));
                int8_t *dst = ins[i].data;
                for (uint32_t j = 0; j < ins[i].length; ++j) {
                    *dst++ = *src + 128;
                    src++;
                }
            }
        }

        mixer_->add_sample(ins[i].data, ins[i].length, 1.0,
            ins[i].flags & 4 ? Sample16Bits : 0);
    }
    instruments_in_place = false;

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
    ordNum = k + 1;

    setspeed(6);
    settempo(125);
    setglobalvol(64);

    amigalimits  = (d.read8(0x26) & 0x10) ? true : false;
    oldstvib     =  d.read8(0x26) & 0x01;
    mastermul    =  d.read8(0x33);
    fastvolslide = (d.read16l(0x28) == 0x1300) || (d.read8(0x26) & 0x40);

    // ... mastermul stuff

    stereomode = mastermul & 128;

    mastermul &= 127;
    if (mastermul == 0) {
        mastermul = 48;  // default in ST3 when you play a song where mastermul=0
    }

    if (d.read8(0x32) > 0)    settempo(d.read8(0x32));
    if (d.read8(0x30) != 255) setglobalvol(d.read8(0x30));

    if ((d.read8(0x31) > 0) && (d.read8(0x31) != 255)) {
        setspeed(d.read8(0x31));
    }

    if (amigalimits) {
        aspdmin =  907 / 2;
        aspdmax = 1712 * 2;
    } else {
        aspdmin = 64;
        aspdmax = 32767;
    }

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

}  // namespace st3play

