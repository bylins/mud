#include "fight.extra.attack.hpp"

#include "chars/character.h"
#include "spells.h"
#include "utils.h"

#include <boost/algorithm/string.hpp>

WeaponMagicalAttack::WeaponMagicalAttack(CHAR_DATA * ch) {ch_=ch;}

void WeaponMagicalAttack::set_attack(CHAR_DATA * ch, CHAR_DATA * victim) 
{
    OBJ_DATA *mag_cont;

    mag_cont = GET_EQ(ch, WEAR_QUIVER);
    if (GET_OBJ_VAL(mag_cont, 2) <= 0) 
    {
            act("Эх какой выстрел мог бы быть, а так колчан пуст.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n пошарил$g в колчане и достал$g оттуда мышь.", FALSE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
            mag_cont->set_val(2,0);     
            return;
    }
    
    mag_single_target(GET_LEVEL(ch), ch, victim, NULL, GET_OBJ_VAL(mag_cont, 0), SAVING_REFLEX);
}

bool WeaponMagicalAttack::set_count_attack(CHAR_DATA * ch)
{
    OBJ_DATA *mag_cont;
    mag_cont = GET_EQ(ch, WEAR_QUIVER);
    //sprintf(buf, "Количество выстрелов %d", get_count());
    //act(buf, TRUE, ch, 0, 0, TO_ROOM | TO_ARENA_LISTEN);
    //выстрел из колчана
    if ((GET_EQ(ch, WEAR_BOTHS)&&(GET_OBJ_TYPE(GET_EQ(ch, WEAR_BOTHS)) == OBJ_DATA::ITEM_WEAPON)) 
            && (GET_OBJ_SKILL(GET_EQ(ch, WEAR_BOTHS)) == SKILL_BOWS )
            && (GET_EQ(ch, WEAR_QUIVER)))
        {
            //если у нас в руках лук и носим колчан
            set_count(get_count()+1);
            
            //по договоренности кадый 4 выстрел магический
            if (get_count() >= 4)
            {
                set_count(0);
                mag_cont->add_val(2,-1);
                return true;
                
            }
            else if ((can_use_feat(ch, DEFT_SHOOTER_FEAT))&&(get_count() == 3))
            {
                set_count(0);
                mag_cont->add_val(2,-1);
                return true;
            }
            return false;
        }
    set_count(0);
    return false;
}
