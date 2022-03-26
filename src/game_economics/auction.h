/**************************************************************************
*   File: Auction.h                                  Part of Bylins       *
*  Usage: Auction headers functions used by the MUD                       *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef AUCTION_HPP_
#define AUCTION_HPP_

extern const int kAuctionPulses;

class ObjData; // to avoid inclusion of obj.hpp
class CharData; // to avoid inclusion of char.hpp

struct AuctionItem {
	int item_id;
	ObjData *item;
	int seller_unique;
	CharData *seller;
	int buyer_unique;
	CharData *buyer;
	int prefect_unique;
	CharData *prefect;
	int cost;
	int tact;
};

// Auction functions  ***************************************************
void showlots(CharData *ch);
bool auction_drive(CharData *ch, char *argument);

void message_auction(char *message, CharData *ch);
void clear_auction(int lot);
void sell_auction(int lot);
void trans_auction(int lot);
void check_auction(CharData *ch, ObjData *obj);
void tact_auction();
AuctionItem *free_auction(int *lotnum);
int obj_on_auction(ObjData *obj);

#define GET_LOT(value) ((auction_lots+(value)))
#define AUCTION_IDENT_PAY 110    //цена за опознание

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
