hero_mt = {
	__newindex = function (table, key, value)
		if key == "name" then
			SetHeroName(table, value)
		elseif key == "owner" then
			MessageBox("Changing hero owner is not implemented yet")
		elseif key == "spellpoints" then
			SetSpellpoints(table, value)
		elseif key == "level" then
			MessageBox("Changing hero level is not implemented yet")
		elseif key == "tempMoraleBonuses" then
			SetHeroTempMoraleBonuses(table, value)
		elseif key == "tempLuckBonuses" then
			SetHeroTempLuckBonuses(table, value)
		elseif key == "mobility" then
			SetHeroMobility(table, value)
		elseif key == "remainingMobility" then
			SetHeroRemainingMobility(table, value)
		elseif key == "sex" then
			SetHeroSex(table, value)
		elseif key == "experience" then
			SetExperiencePoints(table, value)
		elseif key == "attack" then
			SetPrimarySkill(table, PRIMARY_SKILL_ATTACK, value)
		elseif key == "defense" then
			SetPrimarySkill(table, PRIMARY_SKILL_DEFENSE, value)
		elseif key == "spellpower" then
			SetPrimarySkill(table, PRIMARY_SKILL_SPELLPOWER, value)
		elseif key == "knowledge" then
			SetPrimarySkill(table, PRIMARY_SKILL_KNOWLEDGE, value)
		elseif key == "faction" then
			SetHeroFaction(table, value)
		elseif key == "portrait" then
			SetHeroPortrait(table, value)
		else
			MessageBox("This field is not supported")
		end
	end,
	__index = function (t, k)
		if k == "name" then
			return GetHeroName(t)
		elseif k == "owner" then
			return GetHeroOwner(t)
		elseif k == "spellpoints" then
			return GetSpellpoints(t)
		elseif k == "level" then
			return GetHeroLevel(t)
		elseif k == "tempMoraleBonuses" then
			return GetHeroTempMoraleBonuses(t)
		elseif k == "tempLuckBonuses" then
			return GetHeroTempLuckBonuses(t)
		elseif k == "mobility" then
			return GetHeroMobility(t)
		elseif k == "remainingMobility" then
			return GetHeroRemainingMobility(t)
		elseif k == "x" then
			return GetHeroX(t)
		elseif k == "y" then
			return GetHeroY(t)
		elseif k == "sex" then
			return GetHeroSex(t)
		elseif k == "emptyArtifactSlots" then
			return CountEmptyArtifactSlots(t)
		elseif k == "emptyCreatureSlots" then
			return CountEmptyCreatureSlots(t)
		elseif k == "experience" then
			return GetExperiencePoints(t)
		elseif k == "attack" then
			return GetPrimarySkill(t, PRIMARY_SKILL_ATTACK)
		elseif k == "defense" then
			return GetPrimarySkill(t, PRIMARY_SKILL_DEFENSE)
		elseif k == "spellpower" then
			return GetPrimarySkill(t, PRIMARY_SKILL_SPELLPOWER)
		elseif k == "knowledge" then
			return GetPrimarySkill(t, PRIMARY_SKILL_KNOWLEDGE)
		elseif k == "faction" then
			return GetHeroFaction(t);
		elseif k == "skills" then
			return HeroSkillTable(t)
		elseif k == "artifacts" then
			return HeroArtifactTable(t)
		elseif k == "number" then
			return GetHeroNumber(t)
		elseif k == "portrait" then
			return GetHeroPortrait(t)
		else
			return MethodTable(t, k, "hero")
		end
	end
}

town_mt = {
	__newindex = function (table, key, value)
		if key == "name" then
			SetTownName(table, value)
		elseif key == "owner" then
			SetTownOwner(table, value)
		elseif key == "faction" then
			SetTownFaction(table, value)
		elseif key == "builtToday" then
			SetBuildingFlag(table, value)
		else
			MessageBox("This field is not supported")
		end
	end,
	__index = function (t, k)
		if k == "name" then
			return GetTownName(t)
		elseif k == "owner" then
			return GetTownOwner(t)
		elseif k == "faction" then
			return GetTownFaction(t)
		elseif k == "x" then
			return GetTownX(t)
		elseif k == "y" then
			return GetTownY(t)
		elseif k == "hasVisitingHero" then
			return HasVisitingHero(t)
		elseif k == "visitingHero" then
			return GetVisitingHero(t)
		elseif k == "builtToday" then
		    return GetBuildingFlag(t)
		elseif k == "spells" then
			return TownSpellTable(t)
		else
			return MethodTable(t, k, "town")
		end
	end
}

player_mt = {
	__newindex = function (table, key, value)
		if key == "daysLeftWithoutCastle" then
			SetDaysAfterTownLost(table, value);
		else
			MessageBox("This field is not supported");
		end
	end,
	__index = function (t, k)
		if k == "color" then
			return GetPlayerColor(t);
		elseif k == "numHeroes" then
			return GetNumHeroes(t);
		elseif k == "daysLeftWithoutCastle" then
			return GetDaysAfterTownLost(t);
		elseif k == "resources" then
			return PlayerResourceTable(t);
		elseif k == "number" then
			return GetPlayerNumber(t);
		elseif k == "personality" then
			return GetPlayerPersonality(t);
		elseif k == "heroes" then
			return FunctionAsIndex(t, GetHero)
		elseif k == "towns" then
			return FunctionAsIndex(t, GetPlayerTown)
		elseif k == "heroesForHire" then
			return FunctionAsIndex(t, GetHeroForHire)
		else
			return MethodTable(t, k, "player")
		end
	end
}

battleStack_mt = {
	__newindex = function (table, key, value)
		if key == "side" then
			MessageBox("Changing stack side is not implemented yet")
		elseif key == "type" then
			MessageBox("Changing stack type is not implemented yet")
		elseif key == "quantity" then
			SetStackQuantity(table, value)
		elseif key == "initialQuantity" then
			SetStackInitialQuantity(table, value)
		elseif key == "hex" then
			MessageBox("Changing stack hex is not implemented yet")
		elseif key == "morale" then
			SetStackMorale(table, value)
		elseif key == "luck" then
			SetStackLuck(table, value)
		elseif k == "attack" then
			SetStackAttack(table, value)
		elseif k == "defense" then
			SetStackDefense(table, value)
		elseif k == "speed" then
			SetStackSpeed(table, value)
		elseif k == "shots" then
			SetStackShots(table, value)
		elseif k == "hp" then
			SetStackHp(table, value)
		else
			MessageBox("This field is not supported")
		end
	end,
	__index = function (t, k)
		if k == "side" then
			return GetStackSide(t)
		elseif k == "type" then
			return GetStackType(t)
		elseif k == "creatureType" then
			return GetStackType(t)
		elseif k == "quantity" then
			return GetStackQuantity(t)
		elseif k == "initialQuantity" then
			return GetStackInitialQuantity(t)
		elseif k == "hex" then
			return GetStackHex(t)
		elseif k == "morale" then
			return GetStackMorale(t)
		elseif k == "luck" then
			return GetStackLuck(t)
		elseif k == "attack" then
			return GetStackAttack(t)
		elseif k == "defense" then
			return GetStackDefense(t)
		elseif k == "speed" then
			return GetStackSpeed(t)
		elseif k == "shots" then
			return GetStackShots(t)
		elseif k == "hp" then
			return GetStackHp(t)
		else
			return MethodTable(t, k, "battleStack")
		end
	end
}

campaignChoice_mt = {
	__newindex = function (table, key, value)
		MessageBox("Setting campaign choice is not supported")
	end,
	__index = function (t, k)
		if k == "type" then
			return GetCampaignChoiceType()
		elseif k == "field" then
			return GetCampaignChoiceField()
		elseif k == "amount" then
			return GetCampaignChoiceAmount(t)
		else
			MessageBox("This field is not supported")
		end
	end
}

resource_mt = {
	__newindex = function (table, key, value)
		if key == "wood" then
			return SetResource(table.ptrToObject, RESOURCE_WOOD, value);
		elseif key == "mercury" then
			return SetResource(table.ptrToObject, RESOURCE_MERCURY, value);
		elseif key == "ore" then
			return SetResource(table.ptrToObject, RESOURCE_ORE, value);
		elseif key == "sulfur" then
			return SetResource(table.ptrToObject, RESOURCE_SULFUR, value);
		elseif key == "crystal" or key == "crystals" then
			return SetResource(table.ptrToObject, RESOURCE_CRYSTAL, value);
		elseif key == "gems" then
			return SetResource(table.ptrToObject, RESOURCE_GEMS, value);
		elseif key == "gold" then
			return SetResource(table.ptrToObject, RESOURCE_GOLD, value);
		else
			MessageBox("This resource doesn't exist");
		end;
	end;
	__index = function (t, k)
		if k == "wood" then
			return GetResource(t.ptrToObject, RESOURCE_WOOD);
		elseif k == "mercury" then
			return GetResource(t.ptrToObject, RESOURCE_MERCURY);
		elseif k == "ore" then
			return GetResource(t.ptrToObject, RESOURCE_ORE);
		elseif k == "sulfur" then
			return GetResource(t.ptrToObject, RESOURCE_SULFUR);
		elseif k == "crystal" or k == "crystals" then
			return GetResource(t.ptrToObject, RESOURCE_CRYSTAL);
		elseif k == "gems" then
			return GetResource(t.ptrToObject, RESOURCE_GEMS);
		elseif k == "gold" then
			return GetResource(t.ptrToObject, RESOURCE_GOLD);
		else
			MessageBox("This resource doesn't exist");
		end;
	end;

}

skill_mt = {
	__newindex = function (table, key, value)
		if key >= 0 and key <= 13 then
			SetSecondarySkill(table.ptrToObject, key, value);
		else
			MessageBox("This skill doesn't exist");
		end;
	end;
	__index = function (t, k)
		if k >= 0 and k <= 13 then
			return GetSecondarySkill(t.ptrToObject, k);
		else
			MessageBox("This skill doesn't exist");
		end;
	end;
}

artifact_mt = {
	__newindex = function (table, key, value)
		SetArtifactAtIndex(table.ptrToObject, key, value);
	end;
	__index = function (t, k)
		return GetArtifactAtIndex(t.ptrToObject, k);
	end;
}

spell_mt = {
	__index = function (t, k)
		return GetGuildSpell(t.one_up.ptrToObject, t.this_lvl, k);
	end;
	__newindex = function (table, key, value)
		SetGuildSpell(table.one_up.ptrToObject, table.this_lvl, key, value);
	end;
}

funcIndex_mt = {
	__newindex = function (table, key, value)
		MessageBox("Setting this field is not supported")
	end;
	__index = function (t, k)
		return t.func(t.ptrToObj, k)
	end;
}

func_mt = {
	__call = function (tbl, ...)
		return tbl.func(tbl.ptrToObject, ...);
	end;
}

function PlayerResourceTable(player)
	local tbl = {};
	tbl.ptrToObject = player;
	setmetatable(tbl, resource_mt);
	return tbl;
end;

function FunctionAsIndex(obj, func)
	local tbl = {}
	tbl.ptrToObj = obj
	tbl.func = func
	setmetatable(tbl, funcIndex_mt)
	return tbl
end

function HeroSkillTable(hero)
	local tbl = {};
	tbl.ptrToObject = hero;
	setmetatable(tbl, skill_mt);
	return tbl;
end;

function HeroArtifactTable(hero)
	local tbl = {};
	tbl.ptrToObject = hero;
	setmetatable(tbl, artifact_mt);
	return tbl;
end;

function TownSpellTable(town)
	local tbl = {};
	tbl.ptrToObject = town;
	local i;
	for i = 0, 4 do
		tbl[i] = {};
		tbl[i].one_up = tbl;
		tbl[i].this_lvl = i;
		setmetatable(tbl[i], spell_mt);
	end;
	return tbl;
end;

function MethodTable(object, method, objType)
	local tbl = {};
	tbl.ptrToObject = object;
	if objType == "hero" then
		if method == "grantArtifact" then
			tbl.func = GrantArtifact;
		elseif method == "grantSpellScroll" then
			tbl.func = GrantSpellScroll;
		elseif method == "takeArtifact" then
			tbl.func = TakeArtifact;
		elseif method == "hasArtifact" then
			tbl.func = HasArtifact;
		elseif method == "hasSpellScroll" then
			tbl.func = HasSpellScroll;
		elseif method == "hasSpell" then
			tbl.func = HasSpell;
		elseif method == "grantArmy" then
			tbl.func = GrantArmy;
		elseif method == "hasTroop" then
			tbl.func = HasTroop;
		elseif method == "creatureAmount" then
			tbl.func = GetCreatureAmount;
		elseif method == "takeTroop" then
			tbl.func = TakeTroop;
		elseif method == "grantSpell" then
			tbl.func = GrantSpell;
		elseif method == "forgetSpell" then
			tbl.func = ForgetSpell;
		elseif method == "teleport" then
			tbl.func = TeleportHero;
		else
			MessageBox("This field is not supported")
		end;
	elseif objType == "town" then
		if method == "build" then
			tbl.func = BuildInTown;
		elseif method == "getDwelling" then
			tbl.func = GetNumberOfCreatures;
		elseif method == "setDwelling" then
			tbl.func = SetNumberOfCreatures;
		elseif method == "guildSpell" then
			tbl.func = GetGuildSpell;
		elseif method == "setGuildSpell" then
			tbl.func = SetGuildSpell;
		elseif method == "setNumGuildSpells" then
			tbl.func = SetNumGuildSpells;
		elseif method == "disableBuilding" then
			tbl.func = DisallowBuilding;
		elseif method == "hasBuilt" then
			tbl.func = HasBuilt;
		else
			MessageBox("This field is not supported")
		end;
	elseif objType == "player" then
		--if method == "" then
		--else
			MessageBox("This field is not supported")
		--end;
	elseif objType == "battleStack" then
		--if method == "" then
		--else
			MessageBox("This field is not supported")
		--end;
	end;
	setmetatable(tbl, func_mt);
	return tbl;
end;