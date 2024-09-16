#ifndef LIMITS_HPP_
#define LIMITS_HPP_

extern const int kRecallSpellsInterval;

class CharData;
void underwater_check();
void beat_points_update(int pulse);
void hour_update();
void gain_exp_regardless(CharData *ch, int gain);
int CalcManaGain(const CharData *ch);
void obj_point_update();
void exchange_point_update();
void room_point_update();
void point_update();
void EndowExpToChar(CharData *ch, int gain);
void gain_condition(CharData *ch, unsigned condition, int value);
int hit_gain(CharData *ch);
int move_gain(CharData *ch);

#endif // LIMITS_HPP_

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
