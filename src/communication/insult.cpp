/**
 \authors Created by Sventovit
 \date 20.01.2021.
 \brief Команда "оскорбить".
*/

#include "entities/char.h"
#include "handler.h"

void do_insult(CHAR_DATA *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_DUMB)) {
		send_to_char("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return;
	}
	if (*arg) {
		if (!(victim = get_char_vis(ch, arg, FIND_CHAR_ROOM)))
			send_to_char("&KА он вас и не услышит :(&n\r\n", ch);
		else {
			if (victim != ch) {
				sprintf(buf, "&KВы оскорбили %s.&n\r\n", GET_PAD(victim, 3));
				send_to_char(buf, ch);

				switch (number(0, 2)) {
					case 0:
						if (IS_MALE(ch)) {
							if (IS_MALE(victim))
								act("&K$n высмеял$g вашу манеру держать меч !&n",
									false, ch, nullptr, victim, TO_VICT);
							else
								act("&K$n заявил$g, что удел любой женщины - дети, кухня и церковь.&n",
									false, ch, nullptr, victim, TO_VICT);
						} else    // Ch == Woman
						{
							if (IS_MALE(victim))
								act("&K$n заявил$g вам, что у н$s больше... (что $e имел$g в виду?)&n",
									false, ch, nullptr, victim, TO_VICT);
							else
								act("&K$n обьявил$g всем о вашем близком родстве с Бабой-Ягой.&n",
									false, ch, nullptr, victim, TO_VICT);
						}
						break;
					case 1:
						act("&K$n1 чем-то не удовлетворила ваша мама!&n", false,
							ch, nullptr, victim, TO_VICT);
						break;
					default:
						act("&K$n предложил$g вам посетить ближайший хутор!\r\n"
							"$e заявил$g, что там обитают на редкость крупные бабочки.&n",
							false, ch, nullptr, victim, TO_VICT);
						break;
				}    // end switch

				act("&K$n оскорбил$g $N1. СМЕРТЕЛЬНО.&n", true, ch, nullptr, victim, TO_NOTVICT);
			} else    // ch == victim
			{
				send_to_char("&KВы почувствовали себя оскорбленным.&n\r\n", ch);
			}
		}
	} else
		send_to_char("&KВы уверены, что стоит оскорблять такими словами всех?&n\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
