
/* $Id$ */

#include <stdio.h>
#include <stdlib.h>

#include "freecraft.h"
#include "map.h"
#include "unit.h"
#include "rdtsc.h"

#include "region.h"
#include "region_set.h"
#include "region_groups.h"

#define MOVEMENT_TYPE_TRANSPORTER	0
#define MOVEMENT_TYPE_HOVERCRAFT	1
#define NUM_MOVEMENT_TYPES			2

/*
#define TransporterAllowed(flags) (((flags) & MapFieldWaterAllowed || \
								(flags) & MapFieldCoastAllowed) && \
								!((flags) & MapFieldLandAllowed))
*/

typedef struct super_group SuperGroup;
typedef struct movement_type MovementType;

typedef struct region_group {
	struct region_group *NextInSet;
	struct region_group *NextInSGroup[NUM_MOVEMENT_TYPES];
	unsigned short Id;
	SuperGroup *SuperGroup[NUM_MOVEMENT_TYPES];
	unsigned short Passability;		/* the same thing as Region::Passability */
	unsigned NumFields;
	unsigned short NumRegions;
	Region *Regions;
	Region *LastReg;				/* to speed up additions */
} RegGroup;

struct region_group_set {
	short NextId;
	RegGroup *Groups;
};

struct super_group {
	struct super_group *Next;
	unsigned short Id;
	MovementType *MovementType;
	unsigned NumFields;
	unsigned short NumRegions;
	RegGroup *RegGroups;
};

struct movement_type {
	unsigned short AllowedFlags;
	unsigned short ForbiddenFlags;
	unsigned short HighestId;
	SuperGroup *SuperGroups;
};

static MovementType MovementTypes[] = {
	/* MOVEMENT_TYPE_TRANSPORTER */
	{ MapFieldWaterAllowed | MapFieldCoastAllowed, 0, },
	/* MOVEMENT_TYPE_HOVERCRAFT */
	{ MapFieldWaterAllowed | MapFieldCoastAllowed | MapFieldLandAllowed, 
	  MapFieldForest, }
};

local struct region_group_set RegGroupSet;
local int Initialized = 0;

local int  RegGroupSetNextId (void);
local void RegGroupSetAddGroup (RegGroup * );
local void RegGroupSetDeleteGroup (RegGroup * );
local RegGroup *RegGroupSetFindGroup (int );
local RegGroup *RegGroupNew (int );
local void RegGroupMarkRegions (RegGroup * , Region * );
local void RegGroupAddRegion (RegGroup * , Region * );
local void RegGroupDeleteRegion (RegGroup * , Region * );
local void RegGroupAddToSuperGroups (int group_id);
local SuperGroup *SuperGroupNew (void);
local void SuperGroupDestroy (SuperGroup * );
local void SuperGroupAddRegGroup (SuperGroup * , RegGroup * );
local void SuperGroupDeleteRegGroup (SuperGroup * , RegGroup * );
local void SuperGroupMerge (SuperGroup * , SuperGroup * );
local void InitSuperGroups (void);
local void MovementTypeAddSuperGroup (int , SuperGroup * );
local void MovementTypeDeleteSuperGroup (int , SuperGroup * );
local void MovementTypeInitSuperGroups (int );
local int  MovementTypeCheckPassability (int , unsigned short );
local void ExpandGroup (int , SuperGroup * , RegGroup * );

int RegGroupsInitialize (void)
{
	RegGroupSetInitialize ();
	//InitSuperGroups ();
	Initialized = 1;
#if DEBUG
	PrintSummary ();
#endif
	/* TODO: should return something useful */
	return 0;
}

int RegGroupSetInitialize (void)
{
//	int i;
	Region *reg;
//	int num_reg = RegionSetGetNumRegions ();

//	for (i=1; i<=num_reg; i++) {
//		Region *reg = RegionSetFind (i);
	for (reg=AllRegions; reg; reg=reg->Next) {
		RegGroup *group;
		int new_id;

		if (reg->GroupId)
			continue;

		new_id = RegGroupSetNextId ();
		group = RegGroupNew (new_id);
		group->Passability = reg->Passability;
		if ( !group)
			return -1;
		RegGroupMarkRegions (group, reg);
		RegGroupSetAddGroup (group);

		/* If RegGroupSetInitialize() is called for the first time during
		 * map initialization, SuperGroups are not yet created. Their init
		 * code will run soon that will take care of sorting Groups into
		 * the proper SuperGroups so it's neither neccessary nor correct
		 * to try to add the new Group to its SuperGroups.
		 *
		 * On the other hand, if we are called from the MapChanged() callback,
		 * SuperGroup information is fully set up and we just need to insert
		 * the new Group into the system.
		 */
		if (Initialized)
			RegGroupAddToSuperGroups (group->Id);
	}
	/* If we are called from the MapChanged() callback, it can happen that
	 * the map change due to which we are running destroyed all of the Groups
	 * of a specific SuperGroup. This is common e.g. in case of lakes where
	 * transporter SuperGroup consists of just 2 Groups (shore and the water
	 * itself).  In this case the recreated Groups are not adjacent to any
	 * Groups passable by transporters which means that
	 * RegGroupAddToSuperGroups() won't work. That's why we need call
	 * InitSuperGroups() which inspects all of the Groups and if some Group is
	 * passable for e.g. transporters, yet it doesn't belong to any transporter
	 * SuperGroup, a new transporter SuperGroup is created (as it is done
	 * during initialization).
	 */
	InitSuperGroups ();
	return 0;
}

local int RegGroupSetNextId (void)
{
#if DEBUG
	/* if DEBUGging, try to assign the lowest id available, so that it is
	 * more probable that a group destroyed by MapChanged() callback and
	 * immediatelly recreated again will get the same id. This behavior is
	 * less confusing during debugging but is unnecessarily slow for
	 * production.
	 */
	int group_id;
	for (group_id=1; group_id <= RegGroupSet.NextId; group_id++) {
		if ( !RegGroupSetFindGroup (group_id))
			return group_id;	/* this one is free */
	}
#endif
	return ++RegGroupSet.NextId;
}

local void RegGroupSetAddGroup (RegGroup *group)
{
	RegGroup *g;

	if ( !RegGroupSet.Groups) {
		RegGroupSet.Groups = group;
		return;
	}

	for (g=RegGroupSet.Groups; g->NextInSet; g = g->NextInSet)
		;
	g->NextInSet = group;
}

local void RegGroupSetDeleteGroup (RegGroup *group)
{
	RegGroup *g, *p;

	if (group == RegGroupSet.Groups) {
		RegGroupSet.Groups = RegGroupSet.Groups->NextInSet;
		group->NextInSet = NULL;	/* just to be safe */
		return;
	}
		
	for (p=RegGroupSet.Groups, g=p->NextInSet; g; p=g, g=g->NextInSet) {
		if (g != group) {
			continue;
		} else {
			p->NextInSet = g->NextInSet;
			group->NextInSet = NULL;	/* just to be safe */
			break;
		}
	}
}

local RegGroup *RegGroupSetFindGroup (int group_id)
{
	RegGroup *g;

	/* TODO: throw out this linear search and write a balanced tree or hash
	 * implementation. There can easily be hundreds (maybe thousands?)
	 * of groups on some bigger maps.
	 */
	for (g = RegGroupSet.Groups; g; g = g->NextInSet) {
		if (g->Id == group_id)
			return g;
	}
	return NULL;
}

local RegGroup *RegGroupNew (int id)
{
	RegGroup *new;

	new = (RegGroup * )calloc (1, sizeof (RegGroup));
	if (!new)
		return NULL;

	new->Id = id;
	/* the rest implicitly initialized to 0 by calloc */
	return new;
}

void RegGroupDestroy (int groupid)
{
	RegGroup *g = RegGroupSetFindGroup (groupid);
	Region *r;
	int i;

	RegGroupSetDeleteGroup (g);
	for (i=0; i<NUM_MOVEMENT_TYPES; i++)
		if (g->SuperGroup[i])
			SuperGroupDeleteRegGroup (g->SuperGroup[i], g);

	for (r=g->Regions; r; r=r->NextInGroup) {
		r->GroupId = 0;
	}
	free (g);
}

void RegGroupIdMarkRegions (int group_id, Region *reg)
{
	RegGroup *g = RegGroupSetFindGroup (group_id);
	RegGroupMarkRegions (g, reg);
}

local void RegGroupMarkRegions (RegGroup *group, Region *reg)
{
	int num_reg = RegionSetGetNumRegions ();
	struct open {
		int NextFree;
		Region **Regions;
	} Open;

	Open.Regions = (Region ** )malloc (num_reg * sizeof (Region * ));
	if ( !Open.Regions ) {
		return;		/* FIXME */
	}
	Open.NextFree = 0;

	RegGroupAddRegion (group, reg);
	Open.Regions[Open.NextFree++] = reg;

	while (Open.NextFree) {
		Region *r = Open.Regions[--Open.NextFree];
		int i;

		for (i=0; i < r->NumNeighbors; i++) {
			Region *n = r->Neighbors[i].Region;

			if (n->GroupId == 0 && n->Passability == r->Passability) {
				RegGroupAddRegion (group, n);
				Open.Regions[Open.NextFree++] = n;
			}
		}
	}

	free (Open.Regions);
}

local void RegGroupAddRegion (RegGroup *group, Region *reg)
{
	++group->NumRegions;
	group->NumFields += reg->NumFields;
	reg->GroupId = group->Id;
	reg->NextInGroup = NULL;

	if (group->Regions == NULL) {
		group->Regions = reg;
		group->LastReg = reg;
	} else {
		group->LastReg->NextInGroup = reg;
		group->LastReg = reg;
	}
}

void RegGroupIdAddRegion (int group_id, Region *reg)
{
	RegGroup *group = RegGroupSetFindGroup (group_id);
	RegGroupAddRegion (group, reg);
}

local void RegGroupDeleteRegion (RegGroup *group, Region *reg)
{
	Region *r, *p;
	int success=0;

	if (group->Regions == NULL)
		return;

	if (group->Regions == reg) {
		group->Regions = group->Regions->NextInGroup;
		if (reg == group->LastReg)
			group->LastReg = NULL;
		success=1;
	} else {
		for (p=group->Regions, r=p->NextInGroup; r; p=r, r=r->NextInGroup) {
			if (r != reg)
				continue;
			p->NextInGroup = r->NextInGroup;
			if (r == group->LastReg)
				group->LastReg = p;
			success=1;
			break;
		}
	}

	if (success) {
		--group->NumRegions;
		group->NumFields -= reg->NumFields;
		reg->NextInGroup = NULL;
		printf ("Region %d successfully deleted from Group %d.\n",
					reg->RegId, group->Id);
	} else {
		printf ("RegGroupDeleteRegion(): asked to delete Region %d from "
					"Group %d but it's not here.\n", reg->RegId, group->Id);
	}
}

void RegGroupIdDeleteRegion (int group_id, Region *reg)
{
	RegGroup *group = RegGroupSetFindGroup (group_id);
	RegGroupDeleteRegion (group, reg);
}

/* This is called from the MapChanged event handler. It takes a newly
 * (re)created group, goes through all of its regions and looks for their
 * neighbors that belong to a SuperGroup. If all of its neighbors belong to
 * the same SuperGroup, our RegGroup becomes a part of this SuperGroup too.
 * If the neighbors belong to different SuperGroups, those SuperGroups except
 * the first one to be found are destroyed and all of their RegGroups become
 * part of the first SuperGroup.
 */
void RegGroupAddToSuperGroups (int group_id)
{
	RegGroup *g = RegGroupSetFindGroup (group_id);
	Region *r;

	DebugLevel0Fn ("reinitializing group %d.\n", group_id);
	for (r = g->Regions; r; r = r->NextInGroup) {
		int i;

		for (i=0; i < r->NumNeighbors; i++) {
			Region *n = r->Neighbors[i].Region;
			int type;

			for (type=0; type < NUM_MOVEMENT_TYPES; type++) {
				RegGroup *ng;

				/* FIXME: don't run this test *every* iteration!!! */
				if (!MovementTypeCheckPassability (type, g->Passability))
					continue;
				if (n->GroupId==r->GroupId ||
						!MovementTypeCheckPassability (type, n->Passability)) {
					continue;
				}

				ng = RegGroupSetFindGroup (n->GroupId);

				/* this can happen because this function is called from
				 * an inconsistent state during the MapChanged() callback when
				 * regions have been (re)created but not all of them were
				 * neccessarily assigned to Groups yet.
				 */
				if (!ng)
					continue;

				if (ng->SuperGroup[type] == NULL)
					continue;
				if (ng->SuperGroup[type] == g->SuperGroup[type])
					continue;

				if (g->SuperGroup[type] == NULL) {
					ExpandGroup (type, ng->SuperGroup[type], g);
				} else {
					SuperGroupMerge (g->SuperGroup[type], ng->SuperGroup[type]);
					SuperGroupDestroy (ng->SuperGroup[type]);
				}
			}
		}
	}
}

void RegGroupConsistencyCheck (int group_id)
{
	RegGroup *group = RegGroupSetFindGroup (group_id);
	Region *r;
	int num_regs=0;

	if (!group) {
		printf ("RegGroupConsistencyCheck(): Group %d doesn't exist.\n",
					group_id);
		return;
	}

	for (r = group->Regions; r; r = r->NextInGroup) {
		++num_regs;
		if (r->GroupId != group_id) {
			printf ("Region %d is on Group %d's list but has GroupId==%d.\n",
						r->RegId, group_id, r->GroupId);
		}
	}
	if (num_regs != group->NumRegions) {
		printf ("Group %d has wrong number of Regions (%d counted, %d in "
				"descriptor).\n", group_id, num_regs, group->NumRegions);
	}
}

local SuperGroup *SuperGroupNew (void)
{
	SuperGroup *new;

	new = (SuperGroup * )calloc (1, sizeof (SuperGroup));
	return new;
}

local void SuperGroupAddRegGroup (SuperGroup *s, RegGroup *g)
{
	RegGroup *gptr;
	int type = s->MovementType - MovementTypes;

	s->NumRegions += g->NumRegions;
	s->NumFields += g->NumFields;
	g->SuperGroup[type] = s;
	g->NextInSGroup[type] = NULL;

	if (s->RegGroups == NULL)
		s->RegGroups = g;
	else {
		gptr=s->RegGroups;
		for ( ; gptr->NextInSGroup[type]; gptr=gptr->NextInSGroup[type]);
		gptr->NextInSGroup[type] = g;
	}
	DebugLevel0Fn ("added Group %4d to SuperGroup %2d of type %d.\n",
				g->Id, s->Id, type);
}

local void SuperGroupDeleteRegGroup (SuperGroup *s, RegGroup *group)
{
	RegGroup *g, *p;
	int type = s->MovementType - MovementTypes;

	s->NumRegions -= group->NumRegions;
	s->NumFields -= group->NumFields;

	if (group == s->RegGroups) {
		s->RegGroups = s->RegGroups->NextInSGroup[type];
		group->NextInSGroup[type] = NULL;	/* just to be safe */
		return;
	}
		
	p=s->RegGroups;
	g=p->NextInSGroup[type];
	for ( ; g; p=g, g=g->NextInSGroup[type]) {
		if (g == group) {
			p->NextInSGroup[type] = g->NextInSGroup[type];
			group->NextInSGroup[type] = NULL;	/* just to be safe */
			break;
		}
	}
}

int SuperGroupGetNumRegions (Unit *unit, int group_id)
{
	RegGroup *g = RegGroupSetFindGroup (group_id);

	switch (unit->Type->UnitType) {
	case UnitTypeLand:
		return g->NumRegions;
		break;
	case UnitTypeNaval:
		if (unit->Type->Transporter) {
			return g->SuperGroup[MOVEMENT_TYPE_TRANSPORTER]->NumRegions;
		} else {
			return g->NumRegions;
		}
		break;
	case UnitTypeFly:
		printf ("BUG: Air units don't care about RegGroups!\n");
		return 1;
		break;
	default:
		printf ("BUG: Unknown unit->Type->UnitType!!!\n");
		return 1000000;
		break;
	}
}

/* s0 absorbs guts of s1 and leaves s1 as an empty shell */
local void SuperGroupMerge (SuperGroup *s0, SuperGroup *s1)
{
	RegGroup *g;
	int type = s0->MovementType - MovementTypes;
#if  DEBUG
	int type1 = s1->MovementType - MovementTypes;
	DebugCheck (type != type1);
#endif

	s0->NumFields += s1->NumFields;
	s0->NumRegions += s1->NumRegions;

	DebugCheck (!s0->RegGroups);
	for (g=s0->RegGroups; g->NextInSGroup[type]; g=g->NextInSGroup[type]);
	g->NextInSGroup[type] = s1->RegGroups;

	s1->NumRegions = s1->NumFields = 0;
	s1->RegGroups = NULL;
}

local void SuperGroupDestroy (SuperGroup *s)
{
	int type = s->MovementType - MovementTypes;
	RegGroup *g;

	MovementTypeDeleteSuperGroup (type, s);

	/* tell the Groups that they are no longer part of this SuperGroup */

	/* ATTENTION: watch out for leaks!! We don't destroy RegGroups as part
     * of this process of destroying their SuperGroup! (Doing so would make
     * little sense in some situations - Groups don't neccessarily have the
     * same life span as SuperGrous.) If Groups' destruction is desired it
     * must be done separately. */
	for (g=s->RegGroups; g->NextInSGroup[type]; g=g->NextInSGroup[type])
		g->SuperGroup[type] = NULL;
}

local void MovementTypeAddSuperGroup (int type, SuperGroup *s)
{
	SuperGroup *sptr;

	s->Id = ++MovementTypes[type].HighestId;
	s->MovementType = MovementTypes + type;
	if (MovementTypes[type].SuperGroups == NULL)
		MovementTypes[type].SuperGroups = s;
	else {
		for (sptr=MovementTypes[type].SuperGroups; sptr->Next; sptr=sptr->Next);
		sptr->Next = s;
	}
}

local void MovementTypeDeleteSuperGroup (int type, SuperGroup *sg)
{
	SuperGroup *s, *p;

	if (MovementTypes[type].SuperGroups == NULL)
		return;

	if (MovementTypes[type].SuperGroups == sg) {
		MovementTypes[type].SuperGroups = MovementTypes[type].SuperGroups->Next;
		sg->Next = NULL;
		return;
	}

	for (p = MovementTypes[type].SuperGroups, s; s; p=s, s=s->Next)
		if (s == sg) {
			p->Next = s->Next;
			s->Next = NULL;
			return;
		}
}

local int MovementTypeCheckPassability (int type, unsigned short flags)
{
	return (flags & MovementTypes[type].AllowedFlags) &&
			!(flags & MovementTypes[type].ForbiddenFlags);
}

local void MovementTypeInitSuperGroups (int type)
{
	RegGroup *g;
	SuperGroup *s;

	for (g = RegGroupSet.Groups; g; g = g->NextInSet) {
		if ( !MovementTypeCheckPassability (type, g->Passability))
			continue;
		if (g->SuperGroup[type])
			/* this group's been assigned to a supergroup already */
			continue;

		s = SuperGroupNew ();
		MovementTypeAddSuperGroup (type, s);
		ExpandGroup (type, s, g);
	}
}

local void ExpandGroup (int type, SuperGroup *s, RegGroup *g)
{
	Region *r;

	SuperGroupAddRegGroup (s, g);
	/* go through all of the group's Regions and look for those whose
	 * neighbors can be entered by a transporter and belong to
	 * another group */
	for (r = g->Regions; r; r = r->NextInGroup) {
		int i;

		for (i=0; i < r->NumNeighbors; i++) {
			Region *n = r->Neighbors[i].Region;

			if (n->GroupId != r->GroupId &&
						MovementTypeCheckPassability (type, n->Passability)) {
				RegGroup *ng = RegGroupSetFindGroup (n->GroupId);

				/* this can happen because this function can be called from
				 * an inconsistent state during the MapChanged() callback when
				 * regions have been (re)created but not all of them were
				 * neccessarily assigned to Groups yet.
				 */
				if (!ng)
					continue;

				if (ng->SuperGroup[type] == NULL) {
					ng->SuperGroup[type] = g->SuperGroup[type];
					ExpandGroup (type, s, ng);
				} else {
					DebugCheck (ng->SuperGroup[type] != g->SuperGroup[type]);
				}
			}
		}
	}
}

local void InitSuperGroups (void)
{
	unsigned ts1, ts0 = rdtsc ();
	MovementTypeInitSuperGroups (MOVEMENT_TYPE_TRANSPORTER);
	MovementTypeInitSuperGroups (MOVEMENT_TYPE_HOVERCRAFT);
	ts1 = rdtsc ();
	printf ("InitSuperGroups(): %d cycles.\n", ts1-ts0);
}

int RegGroupCheckConnectivity (Unit *unit, int groupid0, int groupid1)
{
	switch (unit->Type->UnitType) {
	case UnitTypeLand:
		/* Land units can only enter fields with MapFieldLandAllowed set
		 * so no two different groups can be connected for these units */
		return (groupid0 != groupid1) ? 0 : 1;
		break;
	case UnitTypeNaval:
		if (unit->Type->Transporter) {
			/* transporters can enter fields with either LandAllowed or
			 * CoastAllowed so we need to check out if both groups are part of
			 * the same supergroup. */
			RegGroup *g0 = RegGroupSetFindGroup (groupid0);
			RegGroup *g1 = RegGroupSetFindGroup (groupid1);
			int MovementType = MOVEMENT_TYPE_TRANSPORTER;

			if (g0->SuperGroup[MovementType] == g1->SuperGroup[MovementType])
				return 1;
			else
				return 0;
		} else {
			/* the same as for land units above holds here */
			return (groupid0 != groupid1) ? 0 : 1;
		}
		break;
	case UnitTypeFly:
		printf ("BUG: Air units don't care about RegGroups!\n");
		return 1;
		break;
	}
	DebugLevel0Fn ("shouldn't have reached end!\n");
	return 0;	/* we shouldn't get here - this is just to make gcc happy */
}

#if DEBUG
local char *mov_type_names[] = { "Transporters", "Hovercrafts" };

local void PrintMovementTypeDescriptor (int );
local void PrintSuperGroupDescriptor (SuperGroup * );

local void PrintSummary (void)
{
	int type;

	for (type=0; type < NUM_MOVEMENT_TYPES; type++)
		PrintMovementTypeDescriptor (type);
}

local void PrintMovementTypeDescriptor (int type)
{
	SuperGroup *sg;

	printf ("%s:\n", mov_type_names[type]);
	for (sg=MovementTypes[type].SuperGroups; sg; sg=sg->Next)
		PrintSuperGroupDescriptor (sg);
}

local void PrintSuperGroupDescriptor (SuperGroup *sg)
{
	RegGroup *g;
	int type = sg->MovementType - MovementTypes;

	printf ("  %d:  ", sg->Id);
	for (g=sg->RegGroups; g; g=g->NextInSGroup[type])
		printf (" %d", g->Id);
	printf ("\n");
}

#endif
