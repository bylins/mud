/**
 \authors Created by Sventovit
 \date 20.01.2021.
 \brief Команда "оскорбить".
*/

#include "entities/char_data.h"
#include "handler.h"

void do_insult(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	CharData *victim;

	one_argument(argument, arg);

	if (!ch->is_npc() && PLR_FLAGGED(ch, EPlrFlag::kDumbed)) {
		SendMsgToChar("Боги наказали вас и вы не можете выражать эмоции!\r\n", ch);
		return;
	}
	if (*arg) {
		if (!(victim = get_char_vis(ch, arg, EFind::kCharInRoom)))
			SendMsgToChar("&KА он вас и не услышит :(&n\r\n", ch);
		else {
			if (victim != ch) {
				sprintf(buf, "&KВы оскорбили %s.&n\r\n", GET_PAD(victim, 3));
				SendMsgToChar(buf, ch);

				switch (number(0, 2)) {
					case 0:
						if (IS_MALE(ch)) {
							if (IS_MALE(victim))
								act("&K$n высмеял$g вашу манеру держать меч !&n",
									false, ch, nullptr, victim, kToVict);
							else
								act("&K$n заявил$g, что удел любой женщины - дети, кухня и церковь.&n",
									false, ch, nullptr, victim, kToVict);
						} else    // Ch == Woman
						{
							if (IS_MALE(victim))
								act("&K$n заявил$g вам, что у н$s больше... (что $e имел$g в виду?)&n",
									false, ch, nullptr, victim, kToVict);
							else
								act("&K$n обьявил$g всем о вашем близком родстве с Бабой-Ягой.&n",
									false, ch, nullptr, victim, kToVict);
						}
						break;
					case 1:
						act("&K$n1 чем-то не удовлетворила ваша мама!&n", false,
							ch, nullptr, victim, kToVict);
						break;
					default:
						act("&K$n предложил$g вам посетить ближайший хутор!\r\n"
							"$e заявил$g, что там обитают на редкость крупные бабочки.&n",
							false, ch, nullptr, victim, kToVict);
						break;
				}    // end switch

				act("&K$n оскорбил$g $N1. СМЕРТЕЛЬНО.&n", true, ch, nullptr, victim, kToNotVict);
			} else    // ch == victim
			{
				SendMsgToChar("&KВы почувствовали себя оскорбленным.&n\r\n", ch);
			}
		}
	} else
		SendMsgToChar("&KВы уверены, что стоит оскорблять такими словами всех?&n\r\n", ch);
}

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
