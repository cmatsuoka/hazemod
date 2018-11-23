#ifndef HAZE_PLAYER_FT101_H_
#define HAZE_PLAYER_FT101_H_

#include <cstdint>
#include "player/pc_player.h"


struct FastTracker: public FormatPlayer {
    FastTracker() : FormatPlayer(
        "ft",
        "FastTracker 1.01 playroutine",
        "Based on the Protracker V2.1A replayer",
        "Claudio Matsuoka",
        { "m.k.", "xchn" }
    ) {}

    haze::Player *new_player(void *, uint32_t, int) override;
};

struct FT_ChannelData {
    uint16_t n_length;
    uint16_t n_loopstart;
    uint16_t n_replen;
    uint8_t  output_volume;
    uint8_t  n_finetune;
    uint16_t output_period;
    uint8_t  n_insnum;
    uint8_t  n_wavecontrol;
    uint8_t  n_vibratopos;
    uint8_t  n_tremolopos;
    uint16_t n_command;
    uint8_t  n_offset;
    uint16_t n_period;
    uint16_t n_wantedperiod;
    uint8_t  n_toneportdirec;
    bool     n_gliss;
    uint16_t n_toneportspeed;
    uint8_t  n_vibratospeed;
    uint8_t  n_vibratodepth;
    uint8_t  n_pattpos;
    uint8_t  n_loopcount;
    uint8_t  n_tremolospeed;
    uint8_t  n_tremolodepth;
    uint8_t  n_volume;

    bool inside_loop;
};

class FT_Player : public PCPlayer {

    uint8_t  ft_speed;
    uint8_t  ft_counter;
    uint8_t  ft_song_pos;
    uint8_t  ft_pbreak_pos;
    bool     ft_pos_jump_flag;
    bool     ft_pbreak_flag;
    uint8_t  ft_patt_del_time;
    uint8_t  ft_patt_del_time_2;
    uint8_t  ft_pattern_pos;
    uint16_t ft_current_pattern;
    uint8_t  cia_tempo;
    bool     position_jump_cmd;

    int channels;
    FT_ChannelData ft_chantemp[8];

    // FastTracker methods
    void ft_play_voice(int, const int);
    void ft_e_commands(const int, uint8_t);
    void ft_position_jump(uint8_t);
    void ft_volume_change(const int, uint8_t);
    void ft_pattern_break(uint8_t);
    void ft_set_speed(uint8_t);
    void ft_fine_porta_up(const int, uint8_t);
    void ft_fine_porta_down(const int, uint8_t);
    void ft_set_gliss_control(const int, uint8_t);
    void ft_set_vibrato_control(const int, uint8_t);
    void ft_jump_loop(const int, uint8_t);
    void ft_set_tremolo_control(const int, uint8_t);
    void ft_retrig_note(const int);
    void ft_volume_fine_up(const int, uint8_t);
    void ft_volume_fine_down(const int, uint8_t);
    void ft_note_cut(const int, uint8_t);
    void ft_pattern_delay(const int chn, uint8_t);
    void ft_check_efx(const int);
    void ft_volume_slide(const int, uint8_t);
    void ft_more_e_commands(const int, uint8_t);
    void ft_arpeggio(const int, uint8_t);
    void ft_porta_up(const int, uint8_t);
    void ft_porta_down(const int, uint8_t);
    void ft_tone_portamento(const int);
    void ft_vibrato(const int, uint8_t);
    void ft_vibrato_2(const int);
    void ft_tone_plus_vol_slide(const int, uint8_t);
    void ft_vibrato_plus_vol_slide(const int, uint8_t);
    void ft_tremolo(const int, uint8_t);
    void ft_retrig_note_2(const int, uint8_t);
    void ft_note_cut_2(const int, uint8_t);
    void ft_note_delay_2(const int, uint8_t);
    void ft_new_row();
    void ft_next_position();
    void ft_no_new_pos_yet();
    void ft_music();
    void ft_no_new_all_channels();
    void ft_get_new_note();


public:
    FT_Player(void *, uint32_t, int);
    ~FT_Player();
    void start() override;
    void play() override;
    void reset() override;
    void frame_info(haze::FrameInfo&) override;
    State save_state() override;
    void restore_state(State const&) override;
};


#endif  // HAZE_PLAYER_FT101_H_

