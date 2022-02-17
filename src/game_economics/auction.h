/**************************************************************************
*   File: Auction.h                                  Part of Bylins       *
*  Usage: Auction headers functions used by the MUD                       *
*                                                                         *
*                                                                         *
*  $Author$                                                        *
*  $Date$                                           *
*  $Revision$                                                       *
************************************************************************ */

#ifndef _AUCTION_HPP_
#define _AUCTION_HPP_

class ObjectData; // to avoid inclusion of obj.hpp
class CharacterData; // to avoid inclusion of char.hpp

struct AuctionItem {
	int item_id;
	ObjectData *item;
	int seller_unique;
	CharacterData *seller;
	int buyer_unique;
	CharacterData *buyer;
	int prefect_unique;
	CharacterData *prefect;
	int cost;
	int tact;
};

// Auction functions  ***************************************************
void showlots(CharacterData *ch);
bool auction_drive(CharacterData *ch, char *argument);

void message_auction(char *message, CharacterData *ch);
void clear_auction(int lot);
void sell_auction(int lot);
void trans_auction(int lot);
void check_auction(CharacterData *ch, ObjectData *obj);
void tact_auction(void);
AuctionItem *free_auction(int *lotnum);
int obj_on_auction(ObjectData *obj);

#define GET_LOT(value) ((auction_lots+value))
#define AUCTION_IDENT_PAY 110    //цена за опознание

#endif

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
