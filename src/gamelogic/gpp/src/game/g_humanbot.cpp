/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"
#include "g_bot.h"

qboolean WeaponIsEmpty( weapon_t weapon, playerState_t ps)
{
	if(ps.Ammo <= 0 && ps.clips <= 0 && !BG_Weapon(weapon)->infiniteAmmo)
		return qtrue;
	else
		return qfalse;
}

float PercentAmmoRemaining( weapon_t weapon, int stats[], playerState_t ps ) {
	int maxAmmo, maxClips;
	float totalMaxAmmo, totalAmmo;

	maxAmmo = BG_Weapon(weapon)->maxAmmo;
	maxClips = BG_Weapon(weapon)->maxClips;
	if(!BG_Weapon(weapon)->infiniteAmmo) {
		if( BG_InventoryContainsUpgrade( UP_BATTPACK, stats ) )
			maxAmmo = (int)( (float)maxAmmo * BATTPACK_MODIFIER );

		totalMaxAmmo = (float) maxAmmo + maxClips * maxAmmo;
		totalAmmo = (float) ps.Ammo + ps.clips * maxAmmo;

		return (float) totalAmmo / totalMaxAmmo;
	} else 
		return 1.0f;
}
//Cmd_Buy_f ripoff, weapon version
void BotBuy(gentity_t *self, weapon_t weapon) {
	if( weapon != WP_NONE )
	{
		//already got this?
		if( BG_InventoryContainsWeapon( weapon, self->client->ps.stats ) )
		{
			return;
		}

		// Only humans can buy stuff
		if( BG_Weapon( weapon )->team != TEAM_HUMANS )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if( !BG_Weapon( weapon )->purchasable )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if( !BG_WeaponAllowedInStage( weapon,(stage_t)g_humanStage.integer ) || !BG_WeaponIsAllowed( weapon ) )
		{
			return;
		}

		//can afford this?
		if( BG_Weapon( weapon )->price > (short)self->client->pers.credit )
		{
			return;
		}

		//have space to carry this?
		if( BG_Weapon( weapon )->slots & BG_SlotsForInventory( self->client->ps.stats ) )
		{
			return;
		}

		// In some instances, weapons can't be changed
		if( !BG_PlayerCanChangeWeapon( &self->client->ps ) )
			return;

		self->client->ps.stats[ STAT_WEAPON ] = weapon;
		self->client->ps.Ammo = BG_Weapon( weapon )->maxAmmo;
		self->client->ps.clips = BG_Weapon( weapon )->maxClips;

		if( BG_Weapon( weapon )->usesEnergy &&
			BG_InventoryContainsUpgrade( UP_BATTPACK, self->client->ps.stats ) )
			self->client->ps.Ammo *= BATTPACK_MODIFIER;

		G_ForceWeaponChange( self, weapon );

		//set build delay/pounce etc to 0
		self->client->ps.stats[ STAT_MISC ] = 0;

		//subtract from funds
		G_AddCreditToClient( self->client, -(short)BG_Weapon( weapon )->price, qfalse );
	} else
		return;
	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
void BotBuy(gentity_t *self, upgrade_t upgrade) {
	qboolean  energyOnly = qfalse;

	// Only give energy from reactors or repeaters
	if( G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
		energyOnly = qfalse;
	else if( upgrade == UP_AMMO &&
		BG_Weapon( (weapon_t)self->client->ps.stats[ STAT_WEAPON ] )->usesEnergy &&
		( G_BuildableRange( self->client->ps.origin, 100, BA_H_REACTOR ) ||
		G_BuildableRange( self->client->ps.origin, 100, BA_H_REPEATER ) ) )
		energyOnly = qtrue;
	else if( upgrade == UP_AMMO && BG_Weapon( (weapon_t)self->client->ps.weapon )->usesEnergy )
		return;

	if( upgrade != UP_NONE )
	{
		//already got this?
		if( BG_InventoryContainsUpgrade( upgrade, self->client->ps.stats ) )
		{
			return;
		}

		//can afford this?
		if( BG_Upgrade( upgrade )->price > (short)self->client->pers.credit )
		{
			return;
		}

		//have space to carry this?
		if( BG_Upgrade( upgrade )->slots & BG_SlotsForInventory( self->client->ps.stats )) {
			return;
		}

		// Only humans can buy stuff
		if( BG_Upgrade( upgrade )->team != TEAM_HUMANS )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if( !BG_Upgrade( upgrade )->purchasable )
		{
			return;
		}

		//are we /allowed/ to buy this?
		if( !BG_UpgradeAllowedInStage( upgrade, (stage_t)g_humanStage.integer ) || !BG_UpgradeIsAllowed( upgrade ) )
		{
			return;
		}

		if( upgrade == UP_AMMO )
			G_GiveClientMaxAmmo( self, energyOnly );
		else
		{
			if( upgrade == UP_BATTLESUIT )
			{
				vec3_t newOrigin;

				if( !G_RoomForClassChange( self, PCL_HUMAN_BSUIT, newOrigin ) )
				{
					return;
				}
				VectorCopy( newOrigin, self->client->ps.origin );
				self->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_BSUIT;
				self->client->pers.classSelection = PCL_HUMAN_BSUIT;
				self->botMind->navQuery = navQuerys[PCL_HUMAN_BSUIT];
				self->client->ps.eFlags ^= EF_TELEPORT_BIT;
			}

			//add to inventory
			BG_AddUpgradeToInventory( upgrade, self->client->ps.stats );
		}

		if( upgrade == UP_BATTPACK )
			G_GiveClientMaxAmmo( self, qtrue );

		//subtract from funds
		G_AddCreditToClient( self->client, -(short)BG_Upgrade( upgrade )->price, qfalse );
	} else
		return;

	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
void BotSellWeapons(gentity_t *self) {
	//no armoury nearby
	if( !G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
	{
		return;
	}
	weapon_t selected = BG_GetPlayerWeapon( &self->client->ps );

	if( !BG_PlayerCanChangeWeapon( &self->client->ps ) )
		return;

	//sell weapons
	for(int  i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		//guard against selling the HBUILD weapons exploit
		if( i == WP_HBUILD && self->client->ps.stats[ STAT_MISC ] > 0 )
		{
			continue;
		}

		if( BG_InventoryContainsWeapon( i, self->client->ps.stats ) &&
			BG_Weapon( (weapon_t)i )->purchasable )
		{
			self->client->ps.stats[ STAT_WEAPON ] = WP_NONE;

			//add to funds
			G_AddCreditToClient( self->client, (short)BG_Weapon((weapon_t) i )->price, qfalse );
		}

		//if we have this weapon selected, force a new selection
		if( i == selected )
			G_ForceWeaponChange( self, WP_NONE );
	}
}
void BotSellAll(gentity_t *self) {
	//no armoury nearby
	if( !G_BuildableRange( self->client->ps.origin, 100, BA_H_ARMOURY ) )
	{
		return;
	}
	BotSellWeapons(self);

	//sell upgrades
	for(int i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		//remove upgrade if carried
		if( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) &&
			BG_Upgrade( (upgrade_t)i )->purchasable )
		{

			// shouldn't really need to test for this, but just to be safe
			if( i == UP_BATTLESUIT )
			{
				vec3_t newOrigin;

				if( !G_RoomForClassChange( self, PCL_HUMAN, newOrigin ) )
				{
					continue;
				}
				VectorCopy( newOrigin, self->client->ps.origin );
				self->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN;
				self->client->pers.classSelection = PCL_HUMAN;
				self->client->ps.eFlags ^= EF_TELEPORT_BIT;
			}

			BG_RemoveUpgradeFromInventory( i, self->client->ps.stats );

			if( i == UP_BATTPACK )
				G_GiveClientMaxAmmo( self, qtrue );

			//add to funds
			G_AddCreditToClient( self->client, (short)BG_Upgrade( (upgrade_t)i )->price, qfalse );
		}
	}
	//update ClientInfo
	ClientUserinfoChanged( self->client->ps.clientNum, qfalse );
}
int BotValueOfWeapons(gentity_t *self) {
	int worth = 0;
	for(int i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if( BG_InventoryContainsWeapon( i, self->client->ps.stats ) )
			worth += BG_Weapon( (weapon_t)i )->price;
	}
	return worth;
}
int BotValueOfUpgrades(gentity_t *self) {
	int worth = 0;
	for( int i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if( BG_InventoryContainsUpgrade( i, self->client->ps.stats ) )
			worth += BG_Upgrade((upgrade_t) i )->price;
	}
	return worth;
}
void BotGetDesiredBuy(gentity_t *self, weapon_t *weapon, upgrade_t (*upgrades)[3], int *numUpgrades) {
	int equipmentPrice = BotValueOfWeapons(self) + BotValueOfUpgrades(self);
	int credits = self->client->ps.persistant[PERS_CREDIT];
	int usableCapital = credits + equipmentPrice;

	//we want to buy a ckit
	if(self->botMind->modus == BOT_MODUS_BUILD) {
		*weapon = WP_HBUILD;
		*numUpgrades = 0;
		return;
	}

	//decide what upgrade(s) to buy
	if(g_humanStage.integer >= 2 && usableCapital >= (PAINSAW_PRICE + BSUIT_PRICE )) {
		(*upgrades)[0] = UP_BATTLESUIT;
		*numUpgrades = 1;
		usableCapital -= BSUIT_PRICE;
	} else if(g_humanStage.integer >= 1 && usableCapital >= (SHOTGUN_PRICE + LIGHTARMOUR_PRICE + HELMET_PRICE)) {
		(*upgrades)[0] = UP_LIGHTARMOUR;
		(*upgrades)[1] = UP_HELMET;
		*numUpgrades = 2;
		usableCapital = usableCapital - (LIGHTARMOUR_PRICE + HELMET_PRICE);
	} else if(g_humanStage.integer >= 0 && usableCapital >= (PAINSAW_PRICE + LIGHTARMOUR_PRICE )) {
		(*upgrades)[0] = UP_LIGHTARMOUR;
		*numUpgrades = 1;
		usableCapital -= LIGHTARMOUR_PRICE;
	} else {
		*numUpgrades = 0;
	}

	//now decide what weapon to buy
	if(g_humanStage.integer >= 2  && usableCapital >= LCANNON_PRICE && g_bot_lcannon.integer) {
		*weapon = WP_LUCIFER_CANNON;
		usableCapital -= LCANNON_PRICE;
	} else if(g_humanStage.integer >= 2 && usableCapital >= CHAINGUN_PRICE && (*upgrades)[0] == UP_BATTLESUIT && g_bot_chaingun.integer) {
		*weapon = WP_CHAINGUN;
		usableCapital -= CHAINGUN_PRICE;
	} else if(g_humanStage.integer >= 1 && g_alienStage.integer < 2 && usableCapital >= FLAMER_PRICE && g_bot_flamer.integer){
		*weapon = WP_FLAMER;
		usableCapital -= FLAMER_PRICE;
	}else if(g_humanStage.integer >= 1 && usableCapital >= PRIFLE_PRICE && g_bot_prifle.integer){
		*weapon = WP_PULSE_RIFLE;
		usableCapital -= PRIFLE_PRICE;
	}else if(g_humanStage.integer >= 0 && usableCapital >= CHAINGUN_PRICE && g_bot_chaingun.integer){
		*weapon = WP_CHAINGUN;
		usableCapital -= CHAINGUN_PRICE;
	}else if(g_humanStage.integer >= 0 && usableCapital >= MDRIVER_PRICE && g_bot_mdriver.integer){
		*weapon = WP_MASS_DRIVER;
		usableCapital -= MDRIVER_PRICE;
	}else if(g_humanStage.integer >= 0 && usableCapital >= LASGUN_PRICE && g_bot_lasgun.integer){
		*weapon = WP_LAS_GUN;
		usableCapital -= LASGUN_PRICE;
	}else if(g_humanStage.integer >= 0 && usableCapital >= SHOTGUN_PRICE && g_bot_shotgun.integer){
		*weapon = WP_SHOTGUN;
		usableCapital -= SHOTGUN_PRICE;
	}else if(g_humanStage.integer >= 0 && usableCapital >= PAINSAW_PRICE && g_bot_painsaw.integer){
		*weapon = WP_PAIN_SAW;
		usableCapital -= PAINSAW_PRICE;
	} else {
		*weapon = WP_MACHINEGUN;
		usableCapital -= RIFLE_PRICE;
	}

	//finally, see if we can buy a battpack 
	if(BG_Weapon(*weapon)->usesEnergy && usableCapital >= BATTPACK_PRICE && g_humanStage.integer >= 1 && (*upgrades)[0] != UP_BATTLESUIT) {
		(*upgrades)[(*numUpgrades)++] = UP_BATTPACK;
		usableCapital -= BATTPACK_PRICE;
	}

	//now test to see if we already have all of these items
	//check if we already have everything
	if(BG_InventoryContainsWeapon((int)*weapon,self->client->ps.stats)) {
		int numContain = 0;
		for(int i=0;i<*numUpgrades;i++) {
			if(BG_InventoryContainsUpgrade((int)(*upgrades)[i],self->client->ps.stats)) {
				numContain++;
			}
		}
		//we have every upgrade we want to buy, and the weapon we want to buy, so test if we need ammo
		if(numContain == *numUpgrades) {
			*numUpgrades = 0;
			for(int i=0;i<3;i++) {
				(*upgrades)[i] = UP_NONE;
			}
			if(PercentAmmoRemaining(BG_PrimaryWeapon(self->client->ps.stats),self->client->ps.stats, self->client->ps) < BOT_LOW_AMMO) {
				(*upgrades)[0] = UP_AMMO;
				*numUpgrades = 1;
			}
			*weapon = WP_NONE;
		}
	}
}
int BotFindBuildingNotBuiltByBot(gentity_t *self, int buildingType) {

	float minDistance = -1;
	int closestBuilding = ENTITYNUM_NONE;
	float newDistance;
	gentity_t *target;
	for( int i = MAX_CLIENTS; i < level.num_entities; ++i ) {
		target = &g_entities[i];
		if(target->s.eType == ET_BUILDABLE) {
			if(g_entities[target->builtBy].botMind)
				continue;
			if(target->s.modelindex == buildingType && (target->buildableTeam == TEAM_ALIENS || (target->powered && target->spawned)) && target->health > 0) {
				newDistance = DistanceSquared(self->s.origin,target->s.origin);
				if( newDistance < minDistance|| minDistance == -1) {
					minDistance = newDistance;
					closestBuilding = i;
				}
			}
		}
	}
	return closestBuilding;
}
//finds buildings powered by a specified building in the list
//also filters out any buildings built by bots
int BotFindBuildingsPoweredBy(gentity_t *powerEntity, gentity_t **buildings, int maxBuildings) {
	int numBuildings = 0;
	memset(buildings,0,sizeof(gentity_t*)*maxBuildings);
	for(int i=MAX_CLIENTS;i<level.num_entities;i++) {
		gentity_t *building = &g_entities[i];
		if(building->s.eType != ET_BUILDABLE)
			continue;
		if(building->buildableTeam != powerEntity->buildableTeam)
			continue;
		if(building == powerEntity)
			continue;
		if(g_entities[building->builtBy].botMind)
			continue;
		if(numBuildings < maxBuildings) {
			buildings[numBuildings++] = building;
		}
	}
	return numBuildings;
}
int BotFindBuildingInList(gentity_t **buildings, int numBuildings, int buildingType) {
	for(int i=0;i<numBuildings;i++) {
		gentity_t *building = buildings[i];

		if(building->s.modelindex == buildingType)

			return building->s.number;
	}
	return ENTITYNUM_NONE;
}

int BotFindBestHDecon(gentity_t *self, buildable_t building, vec3_t origin) {

	gentity_t* buildings[100];

	if(building == BA_H_REACTOR) {
		return G_Reactor()->s.number;
	}

	gentity_t *powerEntity = G_PowerEntityForPoint(origin);
	int numBuildings = BotFindBuildingsPoweredBy(powerEntity,buildings,100);
	//there is nothing providing power here, nothing to decon
	if(powerEntity == NULL) 
		return ENTITYNUM_NONE;

	int bestBuild = BotFindBuildingInList(buildings,numBuildings, BA_H_MGTURRET);
	if(bestBuild == ENTITYNUM_NONE) {
		bestBuild = BotFindBuildingInList(buildings,numBuildings, BA_H_TESLAGEN);
		if(bestBuild == ENTITYNUM_NONE) {
			bestBuild = BotFindBuildingInList(buildings, numBuildings, BA_H_DCC);
			if(bestBuild == ENTITYNUM_NONE) {
				bestBuild = BotFindBuildingInList(buildings, numBuildings, BA_H_MEDISTAT);
				if(bestBuild == ENTITYNUM_NONE) {
					bestBuild = BotFindBuildingInList(buildings,numBuildings, BA_H_ARMOURY);
					if(bestBuild == ENTITYNUM_NONE) {
						bestBuild = BotFindBuildingInList(buildings,numBuildings, BA_H_SPAWN);
					}
				}
			}
		}
	}

	return bestBuild;
}
qboolean BotGetBuildingToBuild(gentity_t *self, vec3_t origin, buildable_t *building) {
	if(!level.botBuildLayout.numBuildings) {
		return qfalse;
	}
	//NOTE: Until alien building is (re)implemented, the only buildings in level.botLayout are ones on the human team

	//check all buildings in the current bot layout and see if we need to build them
	for(int i=0;i<level.botBuildLayout.numBuildings;i++) {
		int block = 0;
		VectorCopy(level.botBuildLayout.buildings[i].origin,origin);
		*building = level.botBuildLayout.buildings[i].type;

		//make sure we have a reactor
		if(!G_Reactor() && *building != BA_H_REACTOR)
			continue;

		//cant build some stuff if we arnt in the right stage
		if(!BG_BuildableAllowedInStage(*building,(stage_t) g_humanStage.integer))
			continue;

		//check if we have enough buildpoints in a location to build something
		if(G_GetBuildPoints(origin,BotGetTeam(self)) < BG_Buildable(*building)->buildPoints) {
			//not enough build points, check if there is something we can decon to make room
			if(BotFindBestHDecon(self, *building, origin) == ENTITYNUM_NONE) {
				continue;
			}
		}

		int num = trap_EntitiesInBox(origin,origin,&block,1);
		if(num == 0 || g_entities[block].s.modelindex != *building) {
			return qtrue;
		}
	}
	return qfalse;
}

botTaskStatus_t BotTaskBuildH(gentity_t *self, usercmd_t *botCmdBuffer) {

	float dist = BG_Class((class_t) self->client->ps.stats[ STAT_CLASS ] )->buildDist;
	vec3_t normal;
	vec3_t forward;
	trace_t trace;
	AngleVectors(self->client->ps.viewangles, forward, NULL,NULL);
	vec3_t mins,maxs;
	buildable_t building;
	vec3_t origin;

	//checks
	if(!BG_InventoryContainsWeapon(WP_HBUILD,self->client->ps.stats))
		return TASK_STOPPED;

	if(!BotGetBuildingToBuild(self, origin, &building))
		return TASK_STOPPED;

	if(self->botMind->bestEnemy.ent)
		return TASK_STOPPED;

	vec3_t targetPos;
	BotGetTargetPos(self->botMind->goal,targetPos);

	if(self->client->ps.weapon != WP_HBUILD)
		G_ForceWeaponChange(self, WP_HBUILD);

	BG_BuildableBoundingBox( building, mins, maxs );

	//check for anything blocking the placement of the building
	trap_Trace(&trace,origin,mins,maxs,origin,-1,MASK_PLAYERSOLID);


	int blockingBuildingNum = ENTITYNUM_NONE; //entity number of a blocking building
	int blockerNum = ENTITYNUM_NONE; //entity number of a non-building blocker
	qboolean tooClose = qfalse; //flag telling if the bot is too close to where the building will be placed

	if(trace.entityNum < ENTITYNUM_MAX_NORMAL) {
		gentity_t *ent = &g_entities[trace.entityNum];

		if(ent->s.eType == ET_BUILDABLE && BotGetTeam(ent) == BotGetTeam(self))
			blockingBuildingNum = trace.entityNum;
		else if(trace.entityNum == self->s.number)
			tooClose = qtrue;
		else
			blockerNum = trace.entityNum;

	}



	//if we dont have enough bp, or we are trying to build a reactor, go and decon something
	if(G_GetBuildPoints(origin, BotGetTeam(self)) < BG_Buildable(building)->buildPoints || (G_Reactor() && building == BA_H_REACTOR)) {

		int deconNum = ENTITYNUM_NONE;
		vec3_t targetPos;
		//set the current goal to best building to decon
		if(BotRoutePermission(self, BOT_TASK_BUILD)) {
			//find the best building to decon
			deconNum = BotFindBestHDecon(self, building,origin);
			if(!BotChangeTarget(self, &g_entities[deconNum],NULL)) {
				return TASK_STOPPED;
			}
		}
		deconNum = BotGetTargetEntityNumber(self->botMind->goal);

		//make sure nothing happened to our decon target
		if(!BotTargetIsEntity(self->botMind->goal)) {
			return TASK_STOPPED;
		}
		BotGetTargetPos(self->botMind->goal,targetPos);

		//head to the building if not close enough to decon
		if(!BotTargetIsVisible(self,self->botMind->goal,MASK_SHOT) || DistanceToGoalSquared(self) > Square(100))
			BotMoveToGoal(self, botCmdBuffer);
		else { 
			BotAimAtLocation(self,targetPos,botCmdBuffer);
			//only decon if our build timer lets us
			if(self->client->ps.stats[STAT_MISC] == 0) {
				gentity_t *block = &g_entities[deconNum];

				//aim at the building and decon it
			
				if( !g_cheats.integer ) // add a bit to the build timer if cheats arnt enabled
				{
					self->client->ps.stats[ STAT_MISC ] +=
						BG_Buildable((buildable_t) block->s.modelindex )->buildTime / 4;
				}
				G_Damage( block, self, self, forward, targetPos,
					block->health, 0, MOD_DECONSTRUCT );
				G_FreeEntity(block);
				self->botMind->needNewGoal = qtrue;
			}
		}
	} else if(!VectorCompare(origin,targetPos) || BotRoutePermission(self, BOT_TASK_BUILD)) {
		//set the target coordinate where we will place the building
		if(!BotChangeTarget(self, NULL, &origin))
			return TASK_STOPPED;
	} else if(DistanceToGoalSquared(self) > Square(dist)) {
		//move to where we will place the building until close enough to place it
		BotMoveToGoal(self, botCmdBuffer);
	} else if(tooClose){
		//we are too close to the building placement, back up and move around to try to get out of the way
		botCmdBuffer->forwardmove = -127;
		botCmdBuffer->rightmove = ((level.time % 10000) < 5000) ? 127 : -127;

	} else if(blockingBuildingNum != ENTITYNUM_NONE){

		gentity_t *block = &g_entities[blockingBuildingNum];
		BotAimAtLocation(self,origin,botCmdBuffer);
		//wait for build timer to run out
		if(self->client->ps.stats[STAT_MISC] == 0) {
			//deecon any building in the way
			if( !g_cheats.integer ) // add a bit to the build timer
			{
				self->client->ps.stats[ STAT_MISC ] +=
					BG_Buildable((buildable_t) block->s.modelindex )->buildTime / 4;
			}
			G_Damage( block, self, self, forward, self->s.origin,
				block->health, 0, MOD_DECONSTRUCT );
			G_FreeEntity( block);
		}
	} else if(blockerNum != ENTITYNUM_NONE) {
		//no room to place building, wait until block clears
		BotAimAtLocation(self,origin,botCmdBuffer);
		botCmdBuffer->forwardmove = 0;
		botCmdBuffer->upmove = 0;
		botCmdBuffer->rightmove = 0;
	} else if(self->client->ps.stats[STAT_MISC] > 0) {
		//aim at the place we will put the building while we wait for our build timer to go down
		BotAimAtLocation(self,origin,botCmdBuffer);
	} else if(self->client->ps.stats[STAT_MISC] == 0) {
		//build the building
		BotAimAtLocation(self,origin,botCmdBuffer);
		if(!g_cheats.integer) {
			self->client->ps.stats[STAT_MISC] += BG_Buildable(building)->buildTime;
		}
		G_Build(self, building, origin,normal,self->client->ps.viewangles);
		self->botMind->currentBuilding++;
		self->botMind->needNewGoal = qtrue;
	}
	return TASK_RUNNING;
}
botTaskStatus_t BotTaskBuy(gentity_t *self, usercmd_t *botCmdBuffer) {
	upgrade_t upgrades[3];
	weapon_t weapon;
	int numUpgrades;
	BotGetDesiredBuy(self, &weapon, &upgrades, &numUpgrades);
	return BotTaskBuy(self, weapon, upgrades, numUpgrades, botCmdBuffer);
}
botTaskStatus_t BotTaskBuy(gentity_t *self, weapon_t weapon, upgrade_t *upgrades, int numUpgrades, usercmd_t *botCmdBuffer) {
	if(!g_bot_buy.integer)
		return TASK_STOPPED;
	if(BotGetTeam(self) != TEAM_HUMANS)
		return TASK_STOPPED;
	if(!self->botMind->closestBuildings.armoury.ent)
		return TASK_STOPPED; //no armoury, so fail
	if(self->botMind->bestEnemy.ent) {
		if(self->botMind->bestEnemy.distance <= BOT_ENGAGE_DIST)
			return TASK_STOPPED;
	}
	//check if we already have everything
	if(BG_InventoryContainsWeapon(weapon,self->client->ps.stats) || weapon == WP_NONE) {
		//cant buy more than 3 upgrades
		if(numUpgrades > 3)
			return TASK_STOPPED;
		if(numUpgrades == 0)
			return TASK_STOPPED;
		int numContain = 0;
		for(int i=0;i<numUpgrades;i++) {
			if(BG_InventoryContainsUpgrade(upgrades[i],self->client->ps.stats)) {
				numContain++;
			}
		}
		//we have every upgrade we want to buy
		if(numContain == numUpgrades)
			return TASK_STOPPED;
	}

	if(BotRoutePermission(self, BOT_TASK_BUY)) {
		if(!BotChangeTarget(self, self->botMind->closestBuildings.armoury.ent, NULL)) 
			return TASK_STOPPED;
	}

	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED;
	}
	if(self->botMind->goal.ent->health <= 0) {
		return TASK_STOPPED;
	}

	if(self->botMind->closestBuildings.armoury.distance > 100) {
		BotMoveToGoal(self, botCmdBuffer);
		return TASK_RUNNING;
	} else {

		if(numUpgrades && upgrades[0] != UP_AMMO)
			BotSellAll(self);
		else if(weapon != WP_NONE)
			BotSellWeapons(self);
		BotBuy(self, weapon);
		for(int i=0;i<numUpgrades;i++) 
			BotBuy(self, upgrades[i]);
		//we have bought the stuff, return
		return TASK_STOPPED;
	}
}
botTaskStatus_t BotTaskHealH(gentity_t *self, usercmd_t *botCmdBuffer) {
	vec3_t targetPos;
	vec3_t myPos;

	//there is no medi we can use
	if(!self->botMind->closestBuildings.medistation.ent) {
		return TASK_STOPPED;
	}
	if(self->client->ps.stats[STAT_TEAM] != TEAM_HUMANS)
		return TASK_STOPPED;

	if(self->botMind->bestEnemy.ent) {
		if(self->botMind->bestEnemy.distance <= BOT_ENGAGE_DIST)
			return TASK_STOPPED;
	}

	//check conditions upon entering task first time
	if(self->botMind->task != BOT_TASK_HEAL) {
		int maxHealth = BG_Class((class_t)self->client->ps.stats[STAT_CLASS])->health;
		float percentHealth = ((float)self->health / maxHealth);
		float distToMedi = Com_Clamp(0,MAX_HEAL_DIST, self->botMind->closestBuildings.medistation.distance);
		if(self->health > BOT_USEMEDKIT_HP  && BG_InventoryContainsUpgrade(UP_MEDKIT,self->client->ps.stats))
			return TASK_STOPPED;
		if(BG_UpgradeIsActive(UP_MEDKIT, self->client->ps.stats))
			return TASK_STOPPED;
		if(percentHealth * distToMedi > distToMedi * percentHealth )
			return TASK_STOPPED;
	}

	//we are fully healed
	if(BG_Class((class_t)self->client->ps.stats[STAT_CLASS])->health <= self->health && BG_InventoryContainsUpgrade(UP_MEDKIT, self->client->ps.stats)) {
		return TASK_STOPPED;
	}

	//find a new route if we have to
	if(BotRoutePermission(self, BOT_TASK_HEAL)) {
		if(!BotChangeTarget(self, self->botMind->closestBuildings.medistation.ent,NULL))
			return TASK_STOPPED;
	}

	//safety check
	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED;
	}

	//the medi has died so signal that the goal is unusable
	if(self->botMind->goal.ent->health <= 0) {
		return TASK_STOPPED;
	}
	//this medi is no longer powered so signal that the goal is unusable
	if(!self->botMind->goal.ent->powered) {
		return TASK_STOPPED;
	}

	BotGetTargetPos(self->botMind->goal, targetPos);
	VectorCopy(self->s.origin,myPos);
	targetPos[2] += BG_BuildableConfig(BA_H_MEDISTAT)->maxs[2];
	myPos[2] += self->r.mins[2]; //mins is negative

	//keep moving to the medi until we are on top of it
	if(DistanceSquared(myPos, targetPos) > Square(BG_BuildableConfig(BA_H_MEDISTAT)->mins[1]))
		BotMoveToGoal(self, botCmdBuffer);
	return TASK_RUNNING;
}
botTaskStatus_t BotTaskRepair(gentity_t *self, usercmd_t *botCmdBuffer) {
	vec3_t selfPos,targetPos;
	vec3_t forward;
	if(!g_bot_repair.integer)
		return TASK_STOPPED;

	if(!self->botMind->closestDamagedBuilding.ent)
		return TASK_STOPPED;

	if(!BG_InventoryContainsWeapon(WP_HBUILD, self->client->ps.stats))
		return TASK_STOPPED;

	if(BotRoutePermission(self, BOT_TASK_REPAIR)) {
		if( !BotChangeTarget(self, self->botMind->closestDamagedBuilding.ent, NULL) )
			return TASK_STOPPED;
	}

	//safety check
	if(!BotTargetIsEntity(self->botMind->goal)) {
		return TASK_STOPPED;
	}

	//the target building has died so signal that the goal is unusable
	if(self->botMind->goal.ent->health <= 0) {
		return TASK_STOPPED;
	}

	//the target has been healed
	if(self->botMind->goal.ent->health >= BG_Buildable((buildable_t)self->botMind->goal.ent->s.modelindex)->health) {
		return TASK_STOPPED;
	}

	if(self->client->ps.weapon != WP_HBUILD)
		G_ForceWeaponChange( self, WP_HBUILD );

	AngleVectors(self->client->ps.viewangles,forward,NULL,NULL);
	BotGetTargetPos(self->botMind->goal, targetPos);
	VectorMA(self->s.origin,self->r.maxs[1],forward,selfPos);

	//move to the damaged building until we are in range
	if(!BotTargetIsVisible(self,self->botMind->goal,MASK_SHOT) || DistanceToGoalSquared(self) > Square(100)) {
		BotMoveToGoal(self, botCmdBuffer);
	} else {
		//aim at the buildable
		BotSlowAim(self, targetPos, 0.5);
		BotAimAtLocation(self,targetPos,botCmdBuffer);
		//gpp automatically heals buildable if close enough and aiming at it
	}
	return TASK_RUNNING;
}
gentity_t* BotFindDamagedFriendlyStructure( gentity_t *self )
{
	//closest building
	gentity_t* closestBuilding = NULL;

	//minimum distance found
	float minDistance = Square(ALIENSENSE_RANGE);

	for( gentity_t *target = &g_entities[MAX_CLIENTS];target<&g_entities[level.num_entities-1]; target++ )
	{
		float distance;

		if(target->s.eType != ET_BUILDABLE)
			continue;
		if(target->buildableTeam != TEAM_HUMANS)
			continue;
		if(target->health >= BG_Buildable((buildable_t)target->s.modelindex)->health)
			continue;
		if(target->health <= 0)
			continue;
		if(!target->spawned || !target->powered)
			continue;

		distance = DistanceSquared(self->s.origin, target->s.origin);
		if( distance <= minDistance ) {
			minDistance = distance;
			closestBuilding = target;
		}
	}
	return closestBuilding;
}


