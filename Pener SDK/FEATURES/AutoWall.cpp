#include "../includes.h"

#include "../UTILS/interfaces.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/IEngine.h"
#include "../SDK/SurfaceData.h"
#include "../SDK/CTrace.h"

#include "../FEATURES/AutoWall.h"

static bool is_autowalling = false;


bool TraceToExit(Vector& end, SDK::trace_t& tr, float x, float y, float z, float x2, float y2, float z2, SDK::trace_t* trace)
{

	typedef bool(__fastcall* TraceToExitFn)(Vector&, SDK::trace_t&, float, float, float, float, float, float, SDK::trace_t*);
	static TraceToExitFn TraceToExit = (TraceToExitFn)UTILS::FindSignature("client_panorama.dll", "55 8B EC 83 EC 30 F3 0F 10 75");
	if (!TraceToExit)
	{
		return false;
	}//(Vector&, trace_t&, float, float, float, float, float, float, trace_t*);
	_asm
	{
		push trace
		push z2
		push y2
		push x2
		push z
		push y
		push x
		mov edx, tr
		mov ecx, end
		call TraceToExit
		add esp, 0x1C
	}
}

bool CAutoWall::HandleBulletPenetration(SDK::CSWeaponInfo *wpn_data, Autowall_Info &data)
{
	SDK::surfacedata_t *enter_surface_data = INTERFACES::PhysicsProps->GetSurfaceData(data.enter_trace.surface.surfaceProps);
	int enter_material = enter_surface_data->game.material;
	float enter_surf_penetration_mod = *(float*)((DWORD)enter_surface_data + 88);

	data.trace_length += data.enter_trace.flFraction * data.trace_length_remaining;
	data.current_damage *= pow((wpn_data->range_modifier), (data.trace_length * 0.002));

	if ((data.trace_length > 3000.f) || (enter_surf_penetration_mod < 0.1f))
		data.penetration_count = 0;

	if (data.penetration_count <= 0)
		return false;

	Vector dummy;
	SDK::trace_t trace_exit;
	if (!TraceToExit(dummy, data.enter_trace, data.enter_trace.end.x, data.enter_trace.end.y, data.enter_trace.end.z, data.direction.x, data.direction.y, data.direction.z, &trace_exit))
		return false;

	SDK::surfacedata_t *exit_surface_data = INTERFACES::PhysicsProps->GetSurfaceData(trace_exit.surface.surfaceProps);
	int exit_material = exit_surface_data->game.material;

	float exit_surf_penetration_mod = *(float*)((DWORD)exit_surface_data + 88);
	float final_damage_modifier = 0.16f;
	float combined_penetration_modifier = 0.0f;

	if (((data.enter_trace.contents & CONTENTS_GRATE) != 0) || (enter_material == 89) || (enter_material == 71))
	{
		combined_penetration_modifier = 3.0f;
		final_damage_modifier = 0.05f;
	}
	else
	{
		combined_penetration_modifier = (enter_surf_penetration_mod + exit_surf_penetration_mod) * 0.5f;
	}

	if (enter_material == exit_material)
	{
		if (exit_material == 87 || exit_material == 85)
			combined_penetration_modifier = 3.0f;
		else if (exit_material == 76)
			combined_penetration_modifier = 2.0f;
	}

	float v34 = fmaxf(0.f, 1.0f / combined_penetration_modifier);
	float v35 = (data.current_damage * final_damage_modifier) + v34 * 3.0f * fmaxf(0.0f, (3.0f / wpn_data->penetration) * 1.25f);
	float thickness = (trace_exit.end - data.enter_trace.end).Length();

	thickness *= thickness;
	thickness *= v34;
	thickness /= 24.0f;

	float lost_damage = fmaxf(0.0f, v35 + thickness);

	if (lost_damage > data.current_damage)
		return false;

	if (lost_damage >= 0.0f)
		data.current_damage -= lost_damage;

	if (data.current_damage < 1.0f)
		return false;

	data.start = trace_exit.end;
	data.penetration_count--;

	return true;
}


CAutoWall::Autowall_Return_Info CAutoWall::CalculateDamage(Vector start, Vector end, SDK::CBaseEntity* from_entity, SDK::CBaseEntity* to_entity)
{
	// default values for return info, in case we need to return abruptly
	Autowall_Return_Info return_info;
	return_info.damage = -1;
	return_info.hitgroup = -1;
	return_info.hit_entity = nullptr;

	Autowall_Info autowall_info;
	autowall_info.penetration_count = 4;
	autowall_info.start = start;
	autowall_info.end = end;
	autowall_info.current_position = start;

	// direction 
	MATH::AngleVectors(UTILS::CalcAngle(start, end), &autowall_info.direction);

	// attacking entity
	if (!from_entity)
		from_entity = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!from_entity)
		return return_info;

	if (to_entity->GetIsDormant())
		return return_info;

	// setup filters
	auto filter_player = SDK::CTraceFilter();
	filter_player.pSkip1 = from_entity;
	autowall_info.filter = &filter_player;

	// weapon
	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(from_entity->GetActiveWeaponIndex()));
	if (!weapon)
		return return_info;

	// weapon data
	auto weapon_info = weapon->get_full_info();
	if (weapon_info == NULL)
		return return_info;

	if (weapon_info == nullptr)
		return return_info;

	if (!weapon_info)
		return return_info;

	autowall_info.current_damage = weapon_info->damage;
	const float range = min(weapon_info->range, (start - end).Length());
	end = start + (autowall_info.direction * range);


	while (autowall_info.current_damage > 0 && autowall_info.penetration_count > 0)
	{
		UTIL_TraceLine(autowall_info.current_position, end, MASK_SHOT | CONTENTS_GRATE, from_entity, 0, &autowall_info.enter_trace);
		UTIL_ClipTraceToPlayers(autowall_info.current_position, autowall_info.current_position + autowall_info.direction * 40.f, MASK_SHOT | CONTENTS_GRATE, autowall_info.filter, &autowall_info.enter_trace);

		// if reached the end
		if (autowall_info.enter_trace.flFraction == 1.f)
		{
			return_info.damage = autowall_info.current_damage;
			return_info.hitgroup = -1;
			return_info.end = autowall_info.enter_trace.end;
			return_info.hit_entity = nullptr;
		}
		// if hit an entity
		if (autowall_info.enter_trace.hitGroup > 0 && autowall_info.enter_trace.hitGroup <= 7 && autowall_info.enter_trace.m_pEnt)
		{
			// checkles gg
			if ((to_entity && autowall_info.enter_trace.m_pEnt != to_entity) ||
				(autowall_info.enter_trace.m_pEnt->GetTeam() == from_entity->GetTeam()))
			{
				if (!CanPenetrate(from_entity, autowall_info, weapon_info))
					break;

				continue;
			}
			 
			ScaleDamage(autowall_info.enter_trace.hitGroup, autowall_info.enter_trace.m_pEnt, weapon_info->armor_ratio, autowall_info.current_damage);

			// fill the return info
			return_info.damage = autowall_info.current_damage;
			return_info.hitgroup = autowall_info.enter_trace.hitGroup;
			return_info.end = autowall_info.enter_trace.end;
			return_info.hit_entity = autowall_info.enter_trace.m_pEnt;

			break;
		}

		// break out of the loop retard
		if (!CanPenetrate(from_entity, autowall_info, weapon_info))
			break;
	}
	return return_info; 
}
bool CAutoWall::CanPenetrate(SDK::CBaseEntity* attacker, Autowall_Info &info, SDK::CSWeaponInfo* weapon_data)
{

	auto enter_surface_data = INTERFACES::PhysicsProps->GetSurfaceData(info.enter_trace.surface.surfaceProps);
	if (!enter_surface_data)
		return true;

	int use_static_values = 0;
	int material = enter_surface_data->game.material;
	int mask = /*GetWeaponID(local_player) == weapon_taser ? 0x1100 : */0x1002;

	// glass and shit gg
	if (info.enter_trace.m_pEnt && !strcmp("CBreakableSurface",
		info.enter_trace.m_pEnt->GetClientClass()->m_pNetworkName))
		*reinterpret_cast<byte*>(uintptr_t(info.enter_trace.m_pEnt + 0x2EC)) = 2;

	is_autowalling = true;
	bool return_value = !HandleBulletPenetration(weapon_data, info);
	is_autowalling = false;
	return return_value;
}
void CAutoWall::ScaleDamage(int hitgroup, SDK::CBaseEntity *player, float weapon_armor_ratio, float &current_damage)
{
	int armor = player->GetArmor();

	switch (hitgroup)
	{
	case HITGROUP_HEAD:
		current_damage *= 4.f;
		break;

	case HITGROUP_CHEST:
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:

		current_damage *= 1.f;
		break;

	case HITGROUP_STOMACH:

		current_damage *= 1.25f;
		break;

	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:

		current_damage *= 0.75f;
		break;
	}

	if (IsArmored(player, armor, hitgroup))
	{
		float v47 = 1.f, armor_bonus_ratio = 0.5f, armor_ratio = weapon_armor_ratio * 0.5f;

		float new_damage = current_damage * armor_ratio;

		if (((current_damage - (current_damage * armor_ratio)) * (v47 * armor_bonus_ratio)) > armor)
			new_damage = current_damage - (armor / armor_bonus_ratio);

		current_damage = new_damage;
	}
}
bool CAutoWall::IsArmored(SDK::CBaseEntity *player, int armorVal, int hitgroup)
{
	bool res = false;

	if (armorVal > 0)
	{
		switch (hitgroup)
		{
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:

			res = true;
			break;

		case HITGROUP_HEAD:

			res = player->HasHelmet();
			break;

		}
	}

	return res;
}

bool CAutoWall::FloatingPointIsVisible(SDK::CBaseEntity* local_player, const Vector &point)
{
	SDK::trace_t Trace;
	Vector end = point;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
	UTIL_TraceLineSig(local_position, end, MASK_SOLID, local_player, 0, &Trace);

	if (Trace.flFraction == 1.0f)
	{
		return true;
	}

	return false;
}
CAutoWall* autowall = new CAutoWall();