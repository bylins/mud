# -*- coding: koi8-r -*-

import smtplib
from email.mime.text import MIMEText

def send_email(smtp_server, smtp_port, smtp_login, smtp_pass, addr_from, addr_to, msg_text, subject):
	serv = smtplib.SMTP(smtp_server, smtp_port)
	serv.login(smtp_login, smtp_pass)
	msg = MIMEText(msg_text, 'plain', 'koi8-r')
	msg['From'] = addr_from
	msg['To'] = addr_to
	msg['Subject'] = subject	
	serv.sendmail(addr_from, addr_to, msg.as_string())
	serv.quit()
