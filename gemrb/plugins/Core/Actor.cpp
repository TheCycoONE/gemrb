/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Actor.cpp,v 1.60 2004/08/19 21:14:25 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TableMgr.h"
#include "Actor.h"
#include "Interface.h"

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static Color green = {
	0x00, 0xff, 0x00, 0xff
};
static Color red = {
	0xff, 0x00, 0x00, 0xff
};
static Color yellow = {
	0xff, 0xff, 0x00, 0xff
};
static Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static Color magenta = {
	0xff, 0x00, 0xff, 0xff
};
/*
static Color green_dark = {
	0x00, 0x80, 0x00, 0xff
};
static Color red_dark = {
	0x80, 0x00, 0x00, 0xff
};
static Color yellow_dark = {
	0x80, 0x80, 0x00, 0xff
};
static Color cyan_dark = {
	0x00, 0x80, 0x80, 0xff
};
*/

Actor::Actor()
	: Moveble( ST_ACTOR )
{
	int i;

	for (i = 0; i < MAX_STATS; i++) {
		BaseStats[i] = 0;
		Modified[i] = 0;
	}
	Dialog[0] = 0;
	SmallPortrait[0] = 0;
	LargePortrait[0] = 0;

	anims = NULL;

	LongName = NULL;
	ShortName = NULL;

	LastTalkedTo = NULL;
	LastAttacker = NULL;
	LastHitter = NULL;
	LastProtecter = NULL;
	LastProtected = NULL;
	LastCommander = NULL;
	LastHelp = NULL;
	LastSeen = NULL;
	LastHeard = NULL;
	LastSummoner = NULL;
	LastDamage = 0;
	LastDamageType = 0;

	InternalFlags = 0;
	inventory.SetInventoryType(INVENTORY_CREATURE);
}

Actor::~Actor(void)
{
	if (anims) {
		delete( anims );
	}
	if (LongName) {
		free( LongName );
	}
	if (ShortName) {
		free( ShortName );
	}
}

void Actor::SetText(char* ptr, unsigned char type)
{
	size_t len = strlen( ptr ) + 1;
	//32 is the maximum possible length of the actor name in the original games
	if(len>32) len=33; 
	if(type!=2) {
		LongName = ( char * ) realloc( LongName, len );
		memcpy( LongName, ptr, len );
	}
	if(type!=1) {
		ShortName = ( char * ) realloc( ShortName, len );
		memcpy( ShortName, ptr, len );
	}
}

void Actor::SetText(int strref, unsigned char type)
{
	if(type!=2) {
		if(LongName) free(LongName);
		LongName = core->GetString( strref );
	}
	if(type!=1) {
		if(ShortName) free(ShortName);
		ShortName = core->GetString( strref );
	}
}

void Actor::SetAnimationID(unsigned short AnimID)
{
	int i;
	char tmp[7];
	sprintf( tmp, "0x%04X", AnimID );

	int AvatarTable = core->LoadTable( "avatars" );
	TableMgr* at = core->GetTable( AvatarTable );
	int RowIndex = at->GetRowIndex( tmp );
	if (RowIndex < 0) {
		char tmp[256];
		sprintf(tmp, "Invalid or nonexistent avatar entry:%04hX\n", AnimID);
		printMessage("CharAnimations",tmp, LIGHT_RED);

		anims = NULL;
		return;
	}
	char* BaseResRef = at->QueryField( RowIndex, BaseStats[IE_ARMOR_TYPE] );
	char* Mirror = at->QueryField( RowIndex, 4 );
	char* Orient = at->QueryField( RowIndex, 5 );
	if (anims) {
		delete( anims );
	}	
	anims = new CharAnimations( BaseResRef, atoi( Orient ), atoi( Mirror ),
					RowIndex );

	int palType = atoi( at->QueryField( RowIndex, 4 ) );

	Color Pal[256];
	memcpy( Pal, anims->Palette, 256 * sizeof( Color ) );
	if (palType < 10) {
		Color* MetalPal = core->GetPalette( BaseStats[IE_METAL_COLOR], 12 );
		Color* MinorPal = core->GetPalette( BaseStats[IE_MINOR_COLOR], 12 );
		Color* MajorPal = core->GetPalette( BaseStats[IE_MAJOR_COLOR], 12 );
		Color* SkinPal = core->GetPalette( BaseStats[IE_SKIN_COLOR], 12 );
		Color* LeatherPal = core->GetPalette( BaseStats[IE_LEATHER_COLOR], 12 );
		Color* ArmorPal = core->GetPalette( BaseStats[IE_ARMOR_COLOR], 12 );
		Color* HairPal = core->GetPalette( BaseStats[IE_HAIR_COLOR], 12 );
		memcpy( &Pal[0x04], MetalPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x10], MinorPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x1C], MajorPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x28], SkinPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x34], LeatherPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x40], ArmorPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x4C], HairPal, 12 * sizeof( Color ) );
		memcpy( &Pal[0x58], &MinorPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x60], &MajorPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x68], &MinorPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x70], &MetalPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x78], &LeatherPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x80], &LeatherPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0x88], &MinorPal[1], 8 * sizeof( Color ) );
		for (i = 0x90; i < 0xA8; i += 0x08)
			memcpy( &Pal[i], &LeatherPal[1], 8 * sizeof( Color ) );
		memcpy( &Pal[0xB0], &SkinPal[1], 8 * sizeof( Color ) );
		for (i = 0xB8; i < 0xFF; i += 0x08)
			memcpy( &Pal[i], &LeatherPal[1], 8 * sizeof( Color ) );
		free( MetalPal );
		free( MinorPal );
		free( MajorPal );
		free( SkinPal );
		free( LeatherPal );
		free( ArmorPal );
		free( HairPal );
	}
	else if (palType == 10) {   // Avatars in PS:T
		int size = 32;
		int dest = 256-ColorsCount*size;
		for (int i = 0; i < ColorsCount; i++) {
			Color* NewPal = core->GetPalette( Colors[i], size );
			memcpy( &Pal[dest], NewPal, size * sizeof( Color ) );
			dest +=size;
			free( NewPal );
		}
	} else {		       
		printf( "Unknown palType %d\n", palType );
	}

	SetCircleSize();
	anims->SetNewPalette( Pal );
}

CharAnimations* Actor::GetAnims()
{
	return anims;
}

/** Returns a Stat value (Base Value + Mod) */
long Actor::GetStat(unsigned int StatIndex)
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	return Modified[StatIndex];
}

void Actor::SetCircleSize()
{
	Color* color;
	if (Modified[IE_UNSELECTABLE]) {
		color = &magenta;
	} else if (Modified[IE_MORALEBREAK] < 0) {
		color = &yellow;
	} else {
		switch (Modified[IE_EA]) {
			case PC:
			case FAMILIAR:
			case ALLY:
			case CONTROLLED:
			case CHARMED:
			case EVILBUTGREEN:
			case GOODCUTOFF:
				color = &green;
				break;

			case ENEMY:
			case GOODBUTRED:
			case EVILCUTOFF:
				color = &red;
				break;
			default:
				color = &cyan;
				break;
		}
	}
	SetCircle( anims->CircleSize, *color );
}

bool Actor::SetStat(unsigned int StatIndex, long Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	Modified[StatIndex] = Value;
	switch (StatIndex) {
		case IE_ANIMATION_ID:
			SetAnimationID( Value );
			break;
		case IE_EA:
		case IE_UNSELECTABLE:
		case IE_MORALEBREAK:
			SetCircleSize();
			break;
		case IE_HITPOINTS:
			if(Value<=0) {
				Die(NULL);
			}
			if(Value>Modified[IE_MAXHITPOINTS]) {
				Modified[IE_HITPOINTS]=Modified[IE_MAXHITPOINTS];
			}
			break;
	}
	return true;
}
long Actor::GetMod(unsigned int StatIndex)
{
	if (StatIndex >= MAX_STATS) {
		return 0xdadadada;
	}
	return Modified[StatIndex] - BaseStats[StatIndex];
}
/** Returns a Stat Base Value */
long Actor::GetBase(unsigned int StatIndex)
{
	if (StatIndex >= MAX_STATS) {
		return 0xffff;
	}
	return BaseStats[StatIndex];
}

/** Sets a Stat Base Value */
bool Actor::SetBase(unsigned int StatIndex, long Value)
{
	if (StatIndex >= MAX_STATS) {
		return false;
	}
	BaseStats[StatIndex] = Value;
	switch (StatIndex) {
		case IE_ANIMATION_ID:
			SetAnimationID( Value );
			break;
		case IE_EA:
		case IE_UNSELECTABLE:
		case IE_MORALEBREAK:
			SetCircleSize();
			break;
		case IE_HITPOINTS:
			if(Value<=0) {
				Die(NULL);
			}
			break;
	}
	return true;
}
/** call this after load, before applying effects */
void Actor::Init()
{
	memcpy( Modified, BaseStats, MAX_STATS * sizeof( *Modified ) );
}
/** implements a generic opcode function, modify modifier
	returns the change
*/
int Actor::NewStat(unsigned int StatIndex, long ModifierValue,
	long ModifierType)
{
	int oldmod = Modified[StatIndex];

	switch (ModifierType) {
		case MOD_ADDITIVE:
			//flat point modifier
			SetStat(StatIndex, Modified[StatIndex]+ModifierValue);
			break;
		case MOD_ABSOLUTE:
			//straight stat change
			SetStat(StatIndex, ModifierValue);
			break;
		case MOD_PERCENT:
			//percentile
			SetStat(StatIndex, BaseStats[StatIndex] * 100 / ModifierValue);
			break;
	}
	return Modified[StatIndex] - oldmod;
}

//returns actual damage
int Actor::Damage(int damage, int damagetype, Actor *hitter)
{
//recalculate damage based on resistances and difficulty level
	NewStat(IE_HITPOINTS,-damage, MOD_ADDITIVE);
	LastDamageType=damagetype;
	LastDamage=damage;
	LastHitter=hitter;
	return damage;
}

void Actor::DebugDump()
{
	printf( "Debugdump of Actor %s:\n", LongName );
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i] && Scripts[i]->script) {
			poi = Scripts[i]->script->GetName();
		}
		printf( "Script %d: %s\n", i, poi );
	}
	printf( "Area:       %.8s\n", Area );
	printf( "Dialog:     %.8s\n", Dialog );
	printf( "Script name:%.32s\n", scriptName );
	printf( "TalkCount:  %ld\n", TalkCount );
	printf( "PartySlot:  %d\n", InParty );
	printf( "Allegiance: %d\n",(int) GetStat(IE_EA) );
	printf( "Visualrange:%d\n", (int) GetStat(IE_VISUALRANGE) );
	printf( "Mod[IE_EA]: %ld\n", Modified[IE_EA]);
	printf( "Mod[IE_ANIMATION_ID]: 0x%04lX\n", Modified[IE_ANIMATION_ID]);
	ieDword tmp=0;
	core->GetGame()->globals->Lookup("APPEARANCE",tmp);
	printf( "Disguise: %d\n", tmp);
	inventory.dump();
	spellbook.dump();
}

void Actor::SetPosition(Map *map, unsigned int XPos, unsigned int YPos, int jump, int radius)
{
	ClearPath();
	XPos/=16;
	YPos/=12;
	if (jump && !GetStat( IE_DONOTJUMP ) && anims->CircleSize) {
		map->AdjustPosition( XPos, YPos, radius );
	}
	MoveTo( ( XPos * 16 ) + 8, ( YPos * 12 ) + 6 );
}

/* this is returning the level of the character for xp calculations 
   later it could calculate with dual/multiclass, 
   also with iwd2's 3rd ed rules, this is why it is a separate function */
int Actor::GetXPLevel(int modified)
{
	if (modified) {
		return Modified[IE_LEVEL];
	}
	return BaseStats[IE_LEVEL];
}

/** maybe this would be more useful if we calculate with the strength too
*/
int Actor::GetEncumbrance()
{
	return inventory.GetWeight();
}

void Actor::Die(Scriptable *killer)
{
	int minhp=Modified[IE_MINHITPOINTS];
	if(minhp) { //can't die
		SetStat(IE_HITPOINTS, minhp);
	}
	InternalFlags|=IF_JUSTDIED;
	if(!InParty) {
		Actor *act=NULL;
		
		if(killer) {
			if(killer->Type==ST_ACTOR) {
				act = (Actor *) killer;
			}
		}
		if(act && act->InParty) {
			//adjust game statistics here
			//game->KillStat(this, killer);
			InternalFlags|=IF_GIVEXP;
		}
	}

	if(Modified[IE_HITPOINTS]<=0) {
		InternalFlags|=IF_REALLYDIED;
	}
        AnimID = IE_ANI_DIE;
}

bool Actor::CheckOnDeath()
{
	if(!(InternalFlags&IF_REALLYDIED) ) return false;
	//don't mess with the already deceased
	if(Modified[IE_STATE_ID]&STATE_DEAD) return false;
	//we need to check animID here, if it has not played the death
	//sequence yet, then we could return now

	if(InternalFlags&IF_GIVEXP) {
		//give experience to party
		core->GetGame()->ShareXP(GetStat(IE_XPVALUE), true );
		//handle reputation here
		//
	}
	DropItem("",0);
	Active = false; //we its scripts here, do we need it?
	Modified[IE_STATE_ID] |= STATE_DEAD;
	if(Modified[IE_MC_FLAGS]&MC_REMOVE_CORPSE) return true;
	if(Modified[IE_MC_FLAGS]&MC_KEEP_CORPSE) return false;
	//if chunked death, then return true
	return false;
}

/* this will create a heap at location, and transfer the item(s) */
void Actor::DropItem(const char *resref, unsigned int flags)
{
	Map *map = core->GetGame()->GetMap(Area);
	inventory.DropItemAtLocation(resref, flags, map, XPos, YPos);
}

bool Actor::ValidTarget(int ga_flags)
{
	switch(ga_flags&GA_ACTION) {
	case GA_PICK:
		if(Modified[IE_STATE_ID] & STATE_CANTSTEAL) return false;
		break;
	case GA_TALK:
		//can't talk to dead
		if(Modified[IE_STATE_ID] & STATE_CANTLISTEN) return false;
		//can't talk to hostile
		if(Modified[IE_EA]>=EVILCUTOFF) return false;
		break;
	}
	if(ga_flags&GA_NO_DEAD) {
		if(InternalFlags&IF_JUSTDIED) return false;
		if(Modified[IE_STATE_ID] & STATE_DEAD) return false;
	}
	if(ga_flags&GA_SELECT) {
		if(Modified[IE_UNSELECTABLE]) return false;
	}
	return true;
}

//returns true if it won't be destroyed with an area
//in this case it shouldn't be saved with the area either
//it will be saved in the savegame
bool Actor::Persistent()
{
	if(InParty) return true;
	if(InternalFlags&IF_FROMGAME) return true;
	return false;
}
