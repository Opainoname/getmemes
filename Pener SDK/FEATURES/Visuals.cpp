#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/IViewRenderBeams.h"
#include "../FEATURES/Backtracking.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CTrace.h"	
#include "../FEATURES/Resolver.h"
#include "../SDK/CGlobalVars.h"
#include "../FEATURES/Visuals.h"
#include "../UTILS/render.h"
#include "../SDK/IVDebugOverlay.h"
#include <string.h>
#include "Aimbot.h"
//--- Misc Variable Initalization ---//
int alpha[65];
CColor breaking;
CColor backtrack;
static bool bPerformed = false, bLastSetting;
float fade_alpha[65];
float dormant_time[65];
CColor main_color;
CColor ammo;

void CVisuals::set_hitmarker_time(float time)
{
	GLOBAL::flHurtTime = time;
}


inline void CrossProduct(const Vector& a, const Vector& b, Vector& result)
{
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
}

__forceinline float VecDotProduct(const Vector& a, const Vector& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

void CVisuals::Draw()
{
	for (int i = 1; i < 65; i++)
	{
		auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!entity)
			continue;

		if (!local_player)
			continue;

		bool is_local_player = entity == local_player;
		bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

		if (is_local_player)
			continue;

		if (is_teammate) 
			continue;

		if (entity->GetHealth() <= 0)
			continue;

		//--- Colors ---//
		int enemy_hp = entity->GetHealth();
		int hp_red = 255 - (enemy_hp * 2.55);
		int hp_green = enemy_hp * 2.55;
		CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);
		CColor dormant_color = CColor(100, 100, 100, alpha[entity->GetIndex()]);
		CColor box_color;
		CColor still_health;
		CColor alt_color;
		CColor zoom_color;
		CColor bomb_color;

		//--- Domant ESP Checks ---//
		if (entity->GetIsDormant())
		{
			if (alpha[entity->GetIndex()] > 0)
				alpha[entity->GetIndex()] -= 0.4;
			main_color = dormant_color;
			still_health = dormant_color;
			alt_color = dormant_color;
			zoom_color = dormant_color;
			breaking = dormant_color;
			backtrack = dormant_color;
			box_color = dormant_color;
			bomb_color = dormant_color;
			ammo = dormant_color;
		}
		else if (!entity->GetIsDormant())
		{
			alpha[entity->GetIndex()] = 255;
			main_color = CColor(255, 255, 255, alpha[entity->GetIndex()]); //heath_color
			still_health = health_color;
			alt_color = BLACK;
			zoom_color = CColor(150, 150, 220, 255);
			breaking = CColor(220, 150, 150);
			backtrack = CColor(155, 220, 150);
			box_color = SETTINGS::settings.box_col;
			bomb_color = CColor(244, 66, 66, 150);
			ammo = CColor(61, 135, 255);
		}

		//--- Entity Related Rendering ---///
		if (SETTINGS::settings.box_bool)
		{
			DrawBox(entity, box_color);
		}
		if (SETTINGS::settings.name_bool) DrawName(entity, main_color, i);
		if (SETTINGS::settings.weap_bool) DrawWeapon(entity, main_color, i);
		if (SETTINGS::settings.info_bool) DrawInfo(entity, main_color, zoom_color);
		if (SETTINGS::settings.health_bool) DrawHealth(entity, still_health, alt_color);
		//if (SETTINGS::settings.weap_bool) DrawAmmo(entity, ammo, alt_color);
		//DrawDirection(entity->GetVecOrigin());
		//DrawHitbox(entity);
		//DrawSkeleton(entity, main_color);
		DrawBarrel(entity);
	}
}

void CVisuals::ClientDraw()
{
	DrawBorderLines();

	if (SETTINGS::settings.spread_bool)
	{
		DrawCrosshair();
	}

	if (SETTINGS::settings.aa_lines) {
		AntiAimLines();
	}


	DrawIndicator();

	ModulateWorld();

	if (SETTINGS::settings.hitmarker) {
		DrawHitmarker();
	}
}

std::string str_to_upper(std::string strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);

	return strToConvert;
}

void CVisuals::AntiAimLines()
{
	Vector src3D, dst3D, forward, src, dst;
	SDK::trace_t tr;
	SDK::Ray_t ray;
	SDK::CTraceFilter filter;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	filter.pSkip1 = local_player;

	MATH::AngleVectors(Vector(0, local_player->GetLowerBodyYaw(), 0), &forward);
	src3D = local_player->GetVecOrigin();
	dst3D = src3D + (forward * 42.f); //replace 50 with the length you want the line to have 

	ray.Init(src3D, dst3D);

	INTERFACES::Trace->TraceRay(ray, 0, &filter, &tr);

	if (!RENDER::WorldToScreen(src3D, src) || !RENDER::WorldToScreen(tr.end, dst))
		return;

	RENDER::DrawLine(src.x, src.y, dst.x, dst.y, CColor(0, 0, 255, 255));

	MATH::AngleVectors(Vector(0, GLOBAL::fake_angles.y, 0), &forward);
	dst3D = src3D + (forward * 42.f); //replace 50 with the length you want the line to have 

	ray.Init(src3D, dst3D);

	INTERFACES::Trace->TraceRay(ray, 0, &filter, &tr);

	if (!RENDER::WorldToScreen(src3D, src) || !RENDER::WorldToScreen(tr.end, dst))
		return;

	RENDER::DrawLine(src.x, src.y, dst.x, dst.y, CColor(255, 0, 0, 255));
}

void CVisuals::DrawBox(SDK::CBaseEntity* entity, CColor color)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		RENDER::DrawEmptyRect(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, color);
		RENDER::DrawEmptyRect((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, CColor(20, 20, 20, alpha[entity->GetIndex()]));
		RENDER::DrawEmptyRect((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, CColor(20, 20, 20, alpha[entity->GetIndex()]));
	}

	/*Vector bbmin; //another way of doing it, doesn't look gae tho
	Vector bbmax;
	Vector screen1;
	Vector screen2;
	entity->GetRenderBounds(bbmin, bbmax);
	Vector pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	Vector top3D = pos3D + Vector(0, 0, bbmax.z + 10);

	if (RENDER::WorldToScreen(pos3D, screen1) && RENDER::WorldToScreen(top3D, screen2))
	{
	float height = screen1.y - screen2.y;
	float width = height / 4;
	RENDER::DrawEmptyRect(screen2.x - width, screen2.y, screen1.x + width, screen1.y, color); //main
	RENDER::DrawEmptyRect(screen2.x - width - 1, screen2.y - 1, screen1.x + width + 1, screen1.y + 1, CColor(20, 20, 20, alpha[entity->GetIndex()])); //outline
	RENDER::DrawEmptyRect(screen2.x - width + 1, screen2.y + 1, screen1.x + width - 1, screen1.y - 1, CColor(20, 20, 20, alpha[entity->GetIndex()])); //inline
	}*/
}

void CVisuals::DrawCorners(SDK::CBaseEntity* entity, CColor color)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		RENDER::DrawEdges(pos.x - width / 2, top.y, (pos.x - width / 2) + width, top.y + height, 8, color);
		RENDER::DrawEdges((pos.x - width / 2) + 1, top.y + 1, (pos.x - width / 2) + width - 1, top.y + height - 1, 8, CColor(20, 20, 20, alpha[entity->GetIndex()]));
		RENDER::DrawEdges((pos.x - width / 2) - 1, top.y - 1, (pos.x - width / 2) + width + 1, top.y + height + 1, 8, CColor(20, 20, 20, alpha[entity->GetIndex()]));
	}
}

void CVisuals::DrawName(SDK::CBaseEntity* entity, CColor color, int index)
{
	SDK::player_info_t ent_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);
	INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		wchar_t buffer[128];
		wsprintfW(buffer, L"%S", ent_info.name);
		if (MultiByteToWideChar(CP_UTF8, 0, ent_info.name, -1, buffer, 128) > 0)
		{
			int height = (pos.y - top.y);
			int width = height / 2;
			RENDER::DrawF(pos.x, top.y - 7, FONTS::visuals_name_font, true, true, color, ent_info.name); //numpad_menu_font
		}
	}
}

float CVisuals::resolve_distance(Vector src, Vector dest)
{
	Vector delta = src - dest;

	float fl_dist = ::sqrtf((delta.Length()));

	if (fl_dist < 1.0f)
		return 1.0f;

	return fl_dist;
}

void CVisuals::DrawDistance(SDK::CBaseEntity* entity, CColor color)
{
	
}

std::string fix_item_name(std::string name)
{
	std::string cname = name;

	if (cname[0] == 'C')
		cname.erase(cname.begin());

	auto startOfWeap = cname.find("Weapon");
	if (startOfWeap != std::string::npos)
		cname.erase(cname.begin() + startOfWeap, cname.begin() + startOfWeap + 6);

	return cname;
}

void CVisuals::DrawWeapon(SDK::CBaseEntity* entity, CColor color, int index)
{
	SDK::player_info_t ent_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);
	INTERFACES::Engine->GetPlayerInfo(index, &ent_info);

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (!weapon)
		return;

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		RENDER::DrawF(pos.x, pos.y + 18, FONTS::visuals_esp_font, true, true, color, fix_item_name(weapon->GetClientClass()->m_pNetworkName));
	}
}

void CVisuals::DrawHealth(SDK::CBaseEntity* entity, CColor color, CColor dormant)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	int enemy_hp = entity->GetHealth();
	int hp_red = 255 - (enemy_hp * 2.55);
	int hp_green = enemy_hp * 2.55;
	CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;

		float offset = (height / 4.f) + 5;
		UINT hp = height - (UINT)((height * enemy_hp) / 100);

		RENDER::DrawEmptyRect((pos.x - width / 2) - 6, top.y - 1, (pos.x - width / 2) - 3, top.y + height + 1, dormant); //intense maths
		RENDER::DrawLine((pos.x - width / 2) - 4, top.y + hp, (pos.x - width / 2) - 4, top.y + height, color); //could have done a rect here,
		RENDER::DrawLine((pos.x - width / 2) - 5, top.y + hp, (pos.x - width / 2) - 5, top.y + height, color); //but fuck it

		if (entity->GetHealth() < 100)
			RENDER::DrawF((pos.x - width / 2) - 4, top.y + hp, FONTS::visuals_esp_font, true, true, main_color, std::to_string(enemy_hp));
	}
}

void CVisuals::DrawAmmo(SDK::CBaseEntity* entity, CColor color, CColor dormant)
{
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	int enemy_hp = entity->GetHealth();
	int hp_red = 255 - (enemy_hp * 2.55);
	int hp_green = enemy_hp * 2.55;
	CColor health_color = CColor(hp_red, hp_green, 1, alpha[entity->GetIndex()]);

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;

	if (is_teammate)
		return;

	auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	if (!c_baseweapon)
		return;

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);

		float offset = (height / 4.f) + 5;
		UINT hp = height - (UINT)((height * 3) / 100);

		auto animLayer = entity->GetAnimOverlay(1);
		if (!animLayer.m_pOwner)
			return;

		auto activity = entity->GetSequenceActivity(animLayer.m_nSequence);

		int iClip = c_baseweapon->GetLoadedAmmo();
		int iClipMax = c_baseweapon->get_full_info()->iMaxClip1;

		float box_w = (float)fabs(height / 2);
		float width;
		if (activity == 967 && animLayer.m_flWeight != 0.f)
		{
			float cycle = animLayer.m_flCycle; // 1 = finished 0 = just started
			width = (((box_w * cycle) / 1.f));
		}
		else
			width = (((box_w * iClip) / iClipMax));

		RENDER::DrawFilledRect((pos.x - box_w / 2), top.y + height + 3, (pos.x - box_w / 2) + box_w + 2, top.y + height + 7, dormant); //outline
		RENDER::DrawFilledRect((pos.x - box_w / 2) + 1, top.y + height + 4, (pos.x - box_w / 2) + width + 1, top.y + height + 6, color); //ammo
	}
}

void CVisuals::DrawInfo(SDK::CBaseEntity* entity, CColor color, CColor alt)
{
	std::vector<std::pair<std::string, CColor>> stored_info;
	Vector min, max;
	entity->GetRenderBounds(min, max);
	Vector pos, pos3D, top, top3D;
	pos3D = entity->GetAbsOrigin() - Vector(0, 0, 10);
	top3D = pos3D + Vector(0, 0, max.z + 10);

	//if (entity->GetHealth() > 0)
		//stored_info.push_back(std::pair<std::string, CColor>("hp: " + std::to_string(entity->GetHealth()), color));

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	//auto weapon = INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex());
	//auto c_baseweapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(entity->GetActiveWeaponIndex()));

	//if (!weapon)
		//return;

	//if (!c_baseweapon)
		//return;

	//stored_info.push_back(std::pair<std::string, CColor>(weapon->GetClientClass()->m_pNetworkName, color));
	//stored_info.push_back(std::pair<std::string, CColor>("ammo: " + std::to_string(c_baseweapon->GetLoadedAmmo()), color));

	if (entity->GetArmor() > 0)
		stored_info.push_back(std::pair<std::string, CColor>(entity->GetArmorName(), color));

	if (entity->GetIsScoped())
		stored_info.push_back(std::pair<std::string, CColor>("zoom", alt));

	if (!using_fake_angles[entity->GetIndex()] && SETTINGS::settings.resolver_bool)
	{

		if (local_player->GetHealth() == 0)
			stored_info.push_back(std::pair<std::string, CColor>("null", color)); //not resolving
		else if (resolve_type[entity->GetIndex()] == 1)
			stored_info.push_back(std::pair<std::string, CColor>("fake 2", color)); //moving
		else if (resolve_type[entity->GetIndex()] == 2)
			stored_info.push_back(std::pair<std::string, CColor>("shuffle correct", color));
		else if (resolve_type[entity->GetIndex()] == 3)
			stored_info.push_back(std::pair<std::string, CColor>("fake 1", color)); //lby updates
		else if (resolve_type[entity->GetIndex()] == 4)
			stored_info.push_back(std::pair<std::string, CColor>("fake 4", color)); //bruteforce
		else if (resolve_type[entity->GetIndex()] == 5)
			stored_info.push_back(std::pair<std::string, CColor>("fake 3", color)); //logged angle
		else if (resolve_type[entity->GetIndex()] == 6)
			stored_info.push_back(std::pair<std::string, CColor>("lby inverse", color));
		else if (resolve_type[entity->GetIndex()] == 7)
			stored_info.push_back(std::pair<std::string, CColor>("fake 5", color)); //backtracking lby
		else
			stored_info.push_back(std::pair<std::string, CColor>("not resolving", color));
	}

	if (RENDER::WorldToScreen(pos3D, pos) && RENDER::WorldToScreen(top3D, top))
	{
		int height = (pos.y - top.y);
		int width = height / 2;
		int i = 0;
		for (auto Text : stored_info)
		{
			RENDER::DrawF((pos.x + width / 2) + 5, top.y + i, FONTS::visuals_esp_font, false, false, Text.second, Text.first); //numpad_menu_font
			i += 8;
		}
	}
}

void CVisuals::DrawSkeleton(SDK::CBaseEntity* entity, CColor color) //wut hek
{
	matrix3x4_t bone_matrix[128];
	if (!entity->SetupBones(bone_matrix, 128, BONE_USED_BY_ANYTHING, 0))
		return;

	auto studio_model = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());
	if (!studio_model)
		return;

	for (int i = 0; i < studio_model->numbones; i++)
	{
		auto bone = studio_model->GetBone(i);
		if (!bone || bone->parent < 0 || !(bone->flags & BONE_USED_BY_HITBOX))
			continue;

		/// 2 bone matrices, 1 for the child and 1 for the parent bone
		matrix3x4_t bone_matrix_1, bone_matrix_2;
		if (bone_matrix)
		{
			bone_matrix_1 = bone_matrix[i];
			bone_matrix_2 = bone_matrix[bone->parent];
		}
		else
		{
			bone_matrix_1 = entity->GetBoneMatrix(i);
			bone_matrix_2 = entity->GetBoneMatrix(bone->parent);
		}

		Vector bone_position_1 = Vector(bone_matrix_1[0][3], bone_matrix_1[1][3], bone_matrix_1[2][3]),
			bone_position_2 = Vector(bone_matrix_2[0][3], bone_matrix_2[1][3], bone_matrix_2[2][3]);

		Vector screen1, screen2;
		if (RENDER::WorldToScreen(bone_position_1, screen1) && RENDER::WorldToScreen(bone_position_2, screen2))
			RENDER::DrawLine(screen1.x, screen1.y, screen2.x, screen2.y, color);
	}
	/*SDK::studiohdr_t* pStudioModel = INTERFACES::ModelInfo->GetStudioModel(entity->GetModel());
	if (!pStudioModel)
		return;

	matrix3x4_t pBoneToWorldOut[128];
	if (!entity->SetupBones(pBoneToWorldOut, 128, BONE_USED_BY_ANYTHING, 0))
		return;

	SDK::mstudiohitboxset_t* pHitboxSet = pStudioModel->GetHitboxSet(0);
	Vector vParent, vChild, sParent, sChild;

	for (int j = 0; j < pStudioModel->numbones; j++)
	{
		SDK::mstudiobone_t* pBone = pStudioModel->GetBone(j);

		if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
		{
			vChild = entity->GetBonePosition(j);
			vParent = entity->GetBonePosition(pBone->parent);

			int iChestBone = 6;  // Parameter of relevant Bone number
			Vector vBreastBone; // New reference Point for connecting many bones
			Vector vUpperDirection = entity->GetBonePosition(iChestBone + 1) - entity->GetBonePosition(iChestBone); // direction vector from chest to neck
			vBreastBone = entity->GetBonePosition(iChestBone) + vUpperDirection / 2;
			Vector vDeltaChild = vChild - vBreastBone; // Used to determine close bones to the reference point
			Vector vDeltaParent = vParent - vBreastBone;

			// Eliminating / Converting all disturbing bone positions in three steps.
			if ((vDeltaParent.Length() < 9 && vDeltaChild.Length() < 9))
				vParent = vBreastBone;

			if (j == iChestBone - 1)
				vChild = vBreastBone;

			if (abs(vDeltaChild.z) < 5 && (vDeltaParent.Length() < 5 && vDeltaChild.Length() < 5) || j == iChestBone)
				continue;

			INTERFACES::DebugOverlay->ScreenPosition(vParent, sParent);
			INTERFACES::DebugOverlay->ScreenPosition(vChild, sChild);

			INTERFACES::Surface->DrawSetColor(CColor(255, 255, 255, 255));
			INTERFACES::Surface->DrawLine(sParent[0], sParent[1], sChild[0], sChild[1]);
		}
	}*/
	/*for (int i = 0; i < pStudioModel->numbones; i++)
	{
		SDK::mstudiobone_t* pBone = pStudioModel->GetBone(i);
		if (!pBone || !(pBone->flags & 256) || pBone->parent == -1)
			continue;

		Vector vBonePos1;
		if (!RENDER::WorldToScreen(Vector(pBoneToWorldOut[i][0][3], pBoneToWorldOut[i][1][3], pBoneToWorldOut[i][2][3]), vBonePos1))
			continue;

		Vector vBonePos2;
		if (!RENDER::WorldToScreen(Vector(pBoneToWorldOut[pBone->parent][0][3], pBoneToWorldOut[pBone->parent][1][3], pBoneToWorldOut[pBone->parent][2][3]), vBonePos2))
			continue;

		RENDER::DrawLine((int)vBonePos1.x, (int)vBonePos1.y, (int)vBonePos2.x, (int)vBonePos2.y, main_color);
	}*/
}

void CVisuals::DrawBulletBeams()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!INTERFACES::Engine->IsInGame() || !local_player)
	{
		Impacts.clear();
		return;
	}

	if (Impacts.size() > 30)
		Impacts.pop_back();

	for (int i = 0; i < Impacts.size(); i++)
	{
		auto current = Impacts.at(i);

		if (!current.pPlayer)
			continue;

		if (current.pPlayer->GetIsDormant())
			continue;

		bool is_local_player = current.pPlayer == local_player;
		bool is_teammate = local_player->GetTeam() == current.pPlayer->GetTeam() && !is_local_player;


		if (current.pPlayer == local_player)
			current.color = SETTINGS::settings.beam1;
		else if (current.pPlayer != local_player && !is_teammate)
			current.color = SETTINGS::settings.beam2;
		else if (current.pPlayer != local_player && is_teammate)
			current.color = SETTINGS::settings.beam3;

		SDK::BeamInfo_t beamInfo;
		beamInfo.m_nType = SDK::TE_BEAMPOINTS;
		beamInfo.m_pszModelName = "sprites/physbeam.vmt";
		beamInfo.m_nModelIndex = -1;
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = 1.f;
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_flFadeLength = 0.0f;
		beamInfo.m_flAmplitude = 2.0f;
		beamInfo.m_flBrightness = 255.f;
		beamInfo.m_flSpeed = 0.2f;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0.f;
		beamInfo.m_flRed = current.color.RGBA[0];
		beamInfo.m_flGreen = current.color.RGBA[1];
		beamInfo.m_flBlue = current.color.RGBA[2];
		beamInfo.m_nSegments = 2;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = 0;

		beamInfo.m_vecStart = current.pPlayer->GetVecOrigin() + current.pPlayer->GetViewOffset();
		beamInfo.m_vecEnd = current.vecImpactPos;

		auto beam = INTERFACES::ViewRenderBeams->CreateBeamPoints(beamInfo);
		if (beam)
			INTERFACES::ViewRenderBeams->DrawBeam(beam);

		//g_pVDebugOverlay->AddBoxOverlay( current.vecImpactPos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), Vector( 0, 0, 0 ), current.color.r(), current.color.g(), current.color.b(), 125, 0.8f );

		Impacts.erase(Impacts.begin() + i);
	}
}

void CVisuals::DrawCrosshair()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());

	if (!weapon)
		return;

	//--- Screen Positioning ---//
	static int screen_x;
	static int screen_y;
	if (screen_x == 0)
	{
		INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
		screen_x /= 2; screen_y /= 2;
	}

	float cone = weapon->GetSpread() + weapon->GetInaccuracy() * 550.f;

	if (cone <= 0.f)
		return;

	RENDER::DrawFilledCircle(screen_x, screen_y, cone, 50, SETTINGS::settings.spread);
}

void CVisuals::DrawIndicator()
{
	//--- LBY Indication Stuff ---//
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	if (menu_hide)
		return;

	float breaking_lby_fraction = fabs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw())) / 180.f;
	float lby_delta = abs(MATH::NormalizeYaw(GLOBAL::real_angles.y - local_player->GetLowerBodyYaw()));

	int screen_width, screen_height;
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);
	if (!SETTINGS::settings.aa_bool)
		return;


	if (SETTINGS::settings.lby_indicator) {
		RENDER::DrawF(10, screen_height - 70, FONTS::visuals_lby_font, false, false, CColor((1.f - breaking_lby_fraction) * 255.f, breaking_lby_fraction * 255.f, 0), "LBY");


		static bool flip;
		if (GetKeyState(SETTINGS::settings.antiaim.breaklby.flipkey))
			flip = true;
		else
			flip = false;

		if (SETTINGS::settings.antiaim.breaklby.type == 1)
		{
			if (!flip)
			{
				RENDER::DrawF((screen_width / 2) + 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(10, 145, 190, 255), ">"); //green
				RENDER::DrawF((screen_width / 2) - 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(255, 255, 255, 255), "<");
			}
			else
			{
				RENDER::DrawF((screen_width / 2) - 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(10, 145, 190, 255), "<"); //green
				RENDER::DrawF((screen_width / 2) + 40, screen_height / 2, FONTS::visuals_side_font, true, true, CColor(255, 255, 255, 255), ">");
			}
		}
	}

	if (SETTINGS::settings.rifk_arrow)
	{
		static const auto fake_color = CColor(255, 0, 0);


		auto client_viewangles = Vector();
		INTERFACES::Engine->GetViewAngles(client_viewangles);

		constexpr auto radius = 80.f;

		const auto screen_center = Vector2D(screen_width / 2.f, screen_height / 2.f);
		const auto fake_rot = DEG2RAD(client_viewangles.y - GLOBAL::fake_angles.y - 90);

		auto draw_arrow = [&](float rot, CColor color) -> void
		{
			std::vector<SDK::Vertex_t> vertices;
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(12)) * (radius - 25.f), screen_center.y + sinf(rot + DEG2RAD(12)) * (radius - 25.f)))); //25
			vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(12)) * (radius - 25.f), screen_center.y + sinf(rot - DEG2RAD(12)) * (radius - 25.f)))); //25
			RENDER::TexturedPolygon(3, vertices, color);
		};

		draw_arrow(fake_rot, fake_color);


		static const auto real_color = CColor(0, 255, 0);

		const auto real_rot = DEG2RAD(client_viewangles.y - GLOBAL::real_angles.y - 90);

		draw_arrow(real_rot, real_color);


		static const auto lby_color = CColor(0, 0, 255);

		const auto lby_rot = DEG2RAD(client_viewangles.y - local_player->GetLowerBodyYaw() - 90);

		draw_arrow(lby_rot, lby_color);
	}

}

/*void CVisuals::ModulateWorld()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	static SDK::ConVar* sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
	sv_skyname->nFlags &= ~FCVAR_CHEAT;

	if (!local_player || !INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame())
		return;

	if (!bPerformed)
	{
		for (auto i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i))
		{
			static SDK::IMaterial* pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World") || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
			{
				if (bLastSetting)
				{
					sv_skyname->SetValue("sky_csgo_night02");
					pMaterial->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, false);
					pMaterial->ColorModulate(0.15, 0.15, 0.15);
				}
				else
				{
					sv_skyname->SetValue("vertigoblue_hdr");
					pMaterial->ColorModulate(1.00, 1.00, 1.00);
				}
			}
		}
	}
	if (bLastSetting != esp_bool)
	{
		bLastSetting = esp_bool;
	}
}*/

void CVisuals::DrawDirection(const Vector& origin) //monarch is the NIGGA
{
	constexpr float radius = 360.0f;
	int width, height;
	INTERFACES::Engine->GetScreenSize(width, height);

	Vector vRealAngles;
	INTERFACES::Engine->GetViewAngles(vRealAngles);

	Vector vForward, vRight, vUp(0.0f, 0.0f, 1.0f);

	MATH::AngleVectors(vRealAngles, &vForward);

	vForward.z = 0.0f;
	MATH::NormalizeAngle(vForward);
	CrossProduct(vUp, vForward, vRight);

	float flFront = VecDotProduct(origin, vForward);
	float flSide = VecDotProduct(origin, vRight);
	float flXPosition = radius * -flSide;
	float flYPosition = radius * -flFront;

	float rotation = INTERFACES::Globals->user_cmd->viewangles.y + 180;

	rotation = atan2(flXPosition, flYPosition) + M_PI;
	rotation *= 180.0f / M_PI;

	float flYawRadians = -(rotation)* M_PI / 180.0f;
	float flCosYaw = cos(flYawRadians);
	float flSinYaw = sin(flYawRadians);

	flXPosition = (int)((width / 2.0f) + (radius * flSinYaw));
	flYPosition = (int)((height / 2.0f) - (radius * flCosYaw));

	RENDER::DrawFilledCircle(flXPosition, flYPosition, 10, 50, CColor(255, 0, 255, 120));
}

void CVisuals::ModulateWorld() //credits to my nigga monarch
{
	static bool nightmode_performed = false, nightmode_lastsetting;

	if (!INTERFACES::Engine->IsConnected() || !INTERFACES::Engine->IsInGame()) 
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (!local_player) 
	{
		if (nightmode_performed)
			nightmode_performed = false;
		return;
	}

	if (nightmode_lastsetting != SETTINGS::settings.night_bool)
	{
		nightmode_lastsetting = SETTINGS::settings.night_bool;
		nightmode_performed = false;
	}

	if (!nightmode_performed) 
	{
		static SDK::ConVar* r_DrawSpecificStaticProp = INTERFACES::cvar->FindVar("r_DrawSpecificStaticProp");
		r_DrawSpecificStaticProp->nFlags &= ~FCVAR_CHEAT;
		r_DrawSpecificStaticProp->SetValue(0);

		static SDK::ConVar* sv_skyname = INTERFACES::cvar->FindVar("sv_skyname");
		sv_skyname->nFlags &= ~FCVAR_CHEAT;

		for (SDK::MaterialHandle_t i = INTERFACES::MaterialSystem->FirstMaterial(); i != INTERFACES::MaterialSystem->InvalidMaterial(); i = INTERFACES::MaterialSystem->NextMaterial(i)) 
		{
			SDK::IMaterial *pMaterial = INTERFACES::MaterialSystem->GetMaterial(i);

			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World")) 
			{
				if (SETTINGS::settings.night_bool)
					pMaterial->ColorModulate(0.08, 0.08, 0.05);
				else
					pMaterial->ColorModulate(1, 1, 1);

				if (SETTINGS::settings.night_bool)
				{
					sv_skyname->SetValue("sky_csgo_night02");
					pMaterial->SetMaterialVarFlag(SDK::MATERIAL_VAR_TRANSLUCENT, false);
					pMaterial->ColorModulate(0.05, 0.05, 0.05);
				}
				else
				{
					sv_skyname->SetValue("vertigoblue_hdr");
					pMaterial->ColorModulate(1.00, 1.00, 1.00);
				}
			}
			else if (strstr(pMaterial->GetTextureGroupName(), "StaticProp")) 
			{
				if (SETTINGS::settings.night_bool)
					pMaterial->ColorModulate(0.28, 0.28, 0.28);
				else 
					pMaterial->ColorModulate(1, 1, 1);
			}
		}
		nightmode_performed = true;
	}
}

void CVisuals::DrawHitmarker()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;

	static int lineSize = 6;

	static float alpha = 0;
	float step = 255.f / 0.3f * INTERFACES::Globals->frametime;


	if (GLOBAL::flHurtTime + 0.4f >= INTERFACES::Globals->curtime)
		alpha = 255.f;
	else
		alpha -= step;

	if (alpha > 0) {
		int screenSizeX, screenCenterX;
		int screenSizeY, screenCenterY;
		INTERFACES::Engine->GetScreenSize(screenSizeX, screenSizeY);

		screenCenterX = screenSizeX / 2;
		screenCenterY = screenSizeY / 2;
		CColor col = CColor(255, 255, 255, alpha);
		RENDER::DrawLine(screenCenterX - lineSize * 2, screenCenterY - lineSize * 2, screenCenterX - (lineSize), screenCenterY - (lineSize), col);
		RENDER::DrawLine(screenCenterX - lineSize * 2, screenCenterY + lineSize * 2, screenCenterX - (lineSize), screenCenterY + (lineSize), col);
		RENDER::DrawLine(screenCenterX + lineSize * 2, screenCenterY + lineSize * 2, screenCenterX + (lineSize), screenCenterY + (lineSize), col);
		RENDER::DrawLine(screenCenterX + lineSize * 2, screenCenterY - lineSize * 2, screenCenterX + (lineSize), screenCenterY - (lineSize), col);
	}
}

void CVisuals::DrawAntiAimSides()
{
	int screen_x;
	int screen_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	screen_x /= 2; screen_y /= 2;

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!local_player)
		return;

	if (local_player->GetHealth() <= 0)
		return;


	if (SETTINGS::settings.aa_side == 0)
		RENDER::DrawWF(screen_x - 25, screen_y, FONTS::visuals_side_font, CColor(0, 255, 0, 255), L"\u25c4");
	if (SETTINGS::settings.aa_side == 1)
		RENDER::DrawWF(screen_x + 25, screen_y, FONTS::visuals_side_font, CColor(0, 255, 0, 255), L"\u25ba");
}

void CVisuals::DrawBorderLines()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	auto weapon = INTERFACES::ClientEntityList->GetClientEntity(local_player->GetActiveWeaponIndex());

	if (!weapon)
		return;

	//--- Screen Positioning ---//
	int screen_x;
	int screen_y;
	int center_x;
	int center_y;
	INTERFACES::Engine->GetScreenSize(screen_x, screen_y);
	INTERFACES::Engine->GetScreenSize(center_x, center_y);
	center_x /= 2; center_y /= 2;

	CColor scope;

	//--- Rendering Scope Lines ---//
	if (local_player->GetIsScoped())
	{
		scope = CColor(0, 0, 0, 255);



		RENDER::DrawLine(0, center_y, screen_x, center_y, scope);
		RENDER::DrawLine(center_x, 0, center_x, screen_y, scope);
	}
}

void CVisuals::DrawBarrel(SDK::CBaseEntity* entity)
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (entity->GetIsDormant())
		return;
	if (!local_player->IsAlive())
		return;

	if (!SETTINGS::settings.radar)
		return;

	Vector screenPos;
	if (UTILS::IsOnScreen(aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_PELVIS), screenPos)) return;

	auto client_viewangles = Vector();
	auto screen_width = 0, screen_height = 0;

	INTERFACES::Engine->GetViewAngles(client_viewangles);
	INTERFACES::Engine->GetScreenSize(screen_width, screen_height);

	auto radius = 450.f;
	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();

	const auto screen_center = Vector(screen_width / 2.f, screen_height / 2.f, 0);
	const auto rot = DEG2RAD(client_viewangles.y - UTILS::CalcAngle(local_position, aimbot->get_hitbox_pos(entity, SDK::HitboxList::HITBOX_PELVIS)).y - 90);

	std::vector<SDK::Vertex_t> vertices;
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot) * radius, screen_center.y + sinf(rot) * radius)));
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot + DEG2RAD(12)) * (radius - 25.f), screen_center.y + sinf(rot + DEG2RAD(12)) * (radius - 25.f)))); //25
	vertices.push_back(SDK::Vertex_t(Vector2D(screen_center.x + cosf(rot - DEG2RAD(12)) * (radius - 25.f), screen_center.y + sinf(rot - DEG2RAD(12)) * (radius - 25.f)))); //25

	RENDER::TexturedPolygon(3, vertices, SETTINGS::settings.radar_col);
}

void CVisuals::DrawBomb(SDK::CBaseEntity* entity)
{
	//:thinking:
}

CVisuals* visuals = new CVisuals();