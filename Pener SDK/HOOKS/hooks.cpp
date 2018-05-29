#include "..\includes.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include "../ImGui/dx9/imgui_dx9.h"
#include "../ImGui/dx9/imgui_impl_dx9.h"
#include "../NewMenu.h"
#include "hooks.h"
#include "../UTILS/interfaces.h"
#include "../UTILS/offsets.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/CInput.h"
#include "../SDK/IClient.h"
#include "../SDK/CPanel.h"
#include "../UTILS/render.h"
#include "../SDK/ConVar.h"
#include "../SDK/CGlowObjectManager.h"
#include "../SDK/IEngine.h"
#include "../SDK/CTrace.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/RecvData.h"
#include "../UTILS/NetvarHookManager.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/ModelRender.h"
#include "../SDK/RenderView.h"
#include "../SDK/CTrace.h"
#include "../SDK/CViewSetup.h"
#include "../SDK/CGlobalVars.h"

#include "../FEATURES/Movement.h"
#include "../FEATURES/Visuals.h"
#include "../FEATURES/Chams.h"
#include "../FEATURES/AntiAim.h"
#include "../FEATURES/Aimbot.h"
#include "../FEATURES/Resolver.h"
#include "../FEATURES/Backtracking.h"
#include "../FEATURES/FakeWalk.h"
#include "../FEATURES/FakeLag.h"
#include "../FEATURES/EnginePred.h"
#include "../MENU/menu_framework.h"

#include <intrin.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

namespace INIT
{
	HMODULE Dll;
	HWND Window;
    WNDPROC OldWindow;
}

//--- Other Globally Used Variables ---///
static bool tick = false;

//--- Declare Signatures and Patterns Here ---///
static auto CAM_THINK = UTILS::FindSignature("client.dll", "85 C0 75 30 38 86");
//SDK::CGlowObjectManager* pGlowObjectManager = (SDK::CGlowObjectManager*)(UTILS::FindSignature("client.dll", "0F 11 05 ? ? ? ? 83 C8 01") + 0x3);
static auto linegoesthrusmoke = UTILS::FindPattern("client.dll", (PBYTE)"\x55\x8B\xEC\x83\xEC\x08\x8B\x15\x00\x00\x00\x00\x0F\x57\xC0", "xxxxxxxx????xxx");

int ground_tick;


Vector OldOrigin;

void AnimFix(SDK::CBaseEntity* entity)
{

	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	bool is_local_player = entity == local_player;
	bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

	if (is_local_player)
		return;



	struct clientanimating_t
	{
		SDK::CBaseAnimating *pAnimating;
		unsigned int	flags;
		clientanimating_t(SDK::CBaseAnimating *_pAnim, unsigned int _flags) : pAnimating(_pAnim), flags(_flags) {}
	};


	clientanimating_t *animating = nullptr;
	int animflags;

	const unsigned int FCLIENTANIM_SEQUENCE_CYCLE = 0x00000001;


	SDK::CAnimationLayer AnimLayer[15];

	int cnt = 15;
	for (int i = 0; i < cnt; i++)
	{
		AnimLayer[i] = entity->GetAnimOverlay(i);
	}

	float flPoseParameter[24];
	float* pose = (float*)((uintptr_t)entity + 0x2764);
	memcpy(&flPoseParameter, pose, sizeof(float) * 24);

	Vector TargetEyeAngles = *entity->GetEyeAnglesPointer();

	bool bForceAnimationUpdate = entity->GetEyeAnglesPointer()->x != TargetEyeAngles.x || entity->GetEyeAnglesPointer()->y != TargetEyeAngles.y;

	if (bForceAnimationUpdate)
	{
		//Update animations and pose parameters
		clientanimating_t *animating = nullptr;
		int animflags;

		//Make sure game is allowed to client side animate. Probably unnecessary
		for (unsigned int i = 0; i < INTERFACES::g_ClientSideAnimationList->count; i++)
		{
			clientanimating_t *tanimating = (clientanimating_t*)INTERFACES::g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
			SDK::CBaseEntity *pAnimEntity = (SDK::CBaseEntity*)tanimating->pAnimating;
			if (pAnimEntity == entity)
			{
				animating = tanimating;
				animflags = tanimating->flags;
				tanimating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
				break;
			}
		}

		//Update animations/poses
		entity->UpdateClientSideAnimation();

		//Restore anim flags
		if (animating)
			animating->flags = animflags;
	}
	for (unsigned int i = 0; i < INTERFACES::g_ClientSideAnimationList->count; i++)
	{
		clientanimating_t *animating = (clientanimating_t*)INTERFACES::g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
		SDK::CBaseEntity *Entity = (SDK::CBaseEntity*)animating->pAnimating;
		if (Entity != local_player && !Entity->GetIsDormant() && Entity->GetHealth() > 0)
		{

			int TickReceivedNetUpdate[65];

			TickReceivedNetUpdate[entity->GetIndex()] = INTERFACES::Globals->tickcount;

			bool HadClientAnimSequenceCycle[65];

			int ClientSideAnimationFlags[65];
			bool IsBreakingLagCompensation[65];
			IsBreakingLagCompensation[entity->GetIndex()] = !Entity->GetIsDormant() && entity->GetVecOrigin().LengthSqr() > (64.0f * 64.0f);

			unsigned int flags = animating->flags;
			ClientSideAnimationFlags[entity->GetIndex()] = flags;
			HadClientAnimSequenceCycle[entity->GetIndex()] = (flags & FCLIENTANIM_SEQUENCE_CYCLE);
			if (HadClientAnimSequenceCycle[entity->GetIndex()])
			{
				if (IsBreakingLagCompensation[entity->GetIndex()] && INTERFACES::Globals->tickcount != TickReceivedNetUpdate[entity->GetIndex()])
				{
					Entity->UpdateClientSideAnimation();
					//Store the new animations
					Entity->CopyPoseParameters(flPoseParameter);
					Entity->CopyAnimLayers(AnimLayer);
				}
			}
		}
	}
	if (is_local_player) {

		for (unsigned int i = 0; i < INTERFACES::g_ClientSideAnimationList->count; i++)
		{
			clientanimating_t *animating = (clientanimating_t*)INTERFACES::g_ClientSideAnimationList->Retrieve(i, sizeof(clientanimating_t));
			SDK::CBaseEntity *Entity = (SDK::CBaseEntity*)animating->pAnimating;
			if (Entity != local_player && !Entity->GetIsDormant() && Entity->GetHealth() > 0)
			{
				bool HadClientAnimSequenceCycle[65];

				int ClientSideAnimationFlags[65];

				unsigned int flags = animating->flags;
				ClientSideAnimationFlags[entity->GetIndex()] = flags;
				HadClientAnimSequenceCycle[entity->GetIndex()] = (flags & FCLIENTANIM_SEQUENCE_CYCLE);

				if (HadClientAnimSequenceCycle[entity->GetIndex()])
				{
					animating->flags |= FCLIENTANIM_SEQUENCE_CYCLE;
				}
			}
		}
	}
}

void ground_ticks()
{
	auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

	if (!local_player)
		return;

	if (local_player->GetFlags() & FL_ONGROUND)
		ground_tick++;
	else
		ground_tick = 0;
}

namespace HOOKS
{
	CreateMoveFn original_create_move;
	PaintTraverseFn original_paint_traverse;
	FrameStageNotifyFn original_frame_stage_notify;
	DrawModelExecuteFn original_draw_model_execute;
	SceneEndFn original_scene_end;
	TraceRayFn original_trace_ray;
	EndSceneResetFn oEndSceneReset;
	EndSceneFn oEndScene;
	SendDatagramFn original_send_datagram;
	OverrideViewFn original_override_view;
	//ToFirstPersonFn original_to_firstperson;
	SvCheatsGetBoolFn original_get_bool;
	do_post_screen_space_effects_t glow;

	VMT::VMTHookManager iclient_hook_manager;
	VMT::VMTHookManager panel_hook_manager;
	VMT::VMTHookManager model_render_hook_manager;
	VMT::VMTHookManager render_view_hook_manager;
	VMT::VMTHookManager trace_hook_manager;
	VMT::VMTHookManager net_channel_hook_manager;
	VMT::VMTHookManager override_view_hook_manager;
	VMT::VMTHookManager input_table_manager;
	VMT::VMTHookManager get_bool_manager;
	VMT::VMTHookManager direct;

	bool __stdcall HookedCreateMove(float sample_input_frametime, SDK::CUserCmd* cmd)
	{
		if (!cmd || cmd->command_number == 0)
			return false;

		uintptr_t* FPointer; __asm { MOV FPointer, EBP }
		byte* SendPacket = (byte*)(*FPointer - 0x1C);

		if (!SendPacket)
			return false;

		GLOBAL::should_send_packet = *SendPacket;
		// S T A R T


		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			slidebitch->do_fakewalk(cmd);

			if (SETTINGS::settings.bhop_bool)
				movement->bunnyhop(cmd);

			if (SETTINGS::settings.strafe_bool)
				movement->autostrafer(cmd);

			for (int i = 1; i <= 65; i++)
			{
				auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);

				if (!entity)
					continue;

				auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

				if (!local_player)
					return;

				bool is_local_player = entity == local_player;
				bool is_teammate = local_player->GetTeam() == entity->GetTeam() && !is_local_player;

				if (is_local_player)
					continue;

				if (is_teammate)
					continue;

				if (entity->GetHealth() <= 0)
					continue;

				if (entity->GetIsDormant())
					continue;

				if (SETTINGS::settings.stop_bool)
					movement->quick_stop(entity, cmd);
			}




			auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
			if (!local_player)
				return false;

			GLOBAL::LocalPlayer = local_player;

			prediction->run_prediction(cmd); //start prediction
			{
				if (SETTINGS::settings.lag_bool)
					fakelag->do_fakelag(cmd);

				//the aimbool stuff is correct, it just disables these features when aimbot is turned off. chill alpine
				aimbot->shoot_enemy(cmd);
				aimbot->fix_recoil(cmd);
				backtracking->backtrack_player(cmd);
				fakelag->do_fakelag(cmd);

				if (SETTINGS::settings.aa_bool)
				{
					antiaim->do_antiaim(cmd);
					antiaim->fix_movement(cmd);

					ground_ticks();
				}

				if (SETTINGS::settings.aa_bool)
				{
					if (!GLOBAL::should_send_packet)
						GLOBAL::real_angles = cmd->viewangles;
					else {
						OldOrigin = local_player->GetAbsOrigin();
						GLOBAL::fake_angles = cmd->viewangles;
					}
				}
				else
				{
					INTERFACES::Engine->GetViewAngles(GLOBAL::real_angles);
					INTERFACES::Engine->GetViewAngles(GLOBAL::fake_angles);
				}
				
				auto Animations = local_player->GetAnimState();
				if (!Animations)
					return false;

				if (Animations->m_iLastClientSideAnimationUpdateFramecount)
					Animations->m_iLastClientSideAnimationUpdateFramecount--;

				if (Animations->m_flLastClientSideAnimationUpdateTime)
					Animations->m_flLastClientSideAnimationUpdateTime -= INTERFACES::Globals->interval_per_tick;
			}
			prediction->end_prediction(cmd); //end prediction
		}


		GLOBAL::cmd = cmd;

		// E N D
		*SendPacket = GLOBAL::should_send_packet;

		UTILS::ClampLemon(cmd->viewangles);

		return false;
	}
	void __stdcall HookedPaintTraverse(int VGUIPanel, bool ForceRepaint, bool AllowForce)
	{
		std::string panel_name = INTERFACES::Panel->GetName(VGUIPanel);

		if (panel_name == "HudZoom")
			return;

		if (panel_name == "MatSystemTopPanel")
		{
			if (FONTS::ShouldReloadFonts())
				FONTS::InitFonts();

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				if (SETTINGS::settings.esp_bool)
				{
					visuals->Draw();
					visuals->ClientDraw();
				}
			}

			//MENU::PPGUI_PP_GUI::Begin();
			//MENU::Do();
			//MENU::PPGUI_PP_GUI::End();

			UTILS::INPUT::input_handler.Update();

			SDK::ConVar* PostProcVar = INTERFACES::cvar->FindVar("mat_postprocess_enable");
			if (PostProcVar->fValue != 0)
				PostProcVar->SetValue("0");

			RENDER::DrawPixel(1, 1, CColor(0, 0, 0));

		}

		original_paint_traverse(INTERFACES::Panel, VGUIPanel, ForceRepaint, AllowForce);
	}
	void __fastcall HookedFrameStageNotify(void* ecx, void* edx, int stage)
	{
		Vector*pPunchAngle = nullptr, *pViewPunchAngle = nullptr, vPunchAngle, vViewPunchAngle;
		switch (stage)
		{
		case FRAME_NET_UPDATE_POSTDATAUPDATE_START:

			//--- Angle Resolving and Such ---//
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
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

					
					if (SETTINGS::settings.resolver_bool)
						resolver->resolve(entity);
				}
			}
			break;

		case FRAME_NET_UPDATE_POSTDATAUPDATE_END:

			break;

		case FRAME_RENDER_START:

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
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

						auto Animationss = entity->GetAnimState();
					if (Animationss->m_iLastClientSideAnimationUpdateFramecount)
						Animationss->m_iLastClientSideAnimationUpdateFramecount--;

					if (Animationss->m_flLastClientSideAnimationUpdateTime)
						Animationss->m_flLastClientSideAnimationUpdateTime -= INTERFACES::Globals->interval_per_tick;


					AnimFix(entity);
				}

				auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

				if (!local_player)
					return;

				if (SETTINGS::settings.no_vis_recoil) {
					pPunchAngle = local_player->GetPunchAngles();
					pViewPunchAngle = local_player->GetPunchAngles2();

					if (pPunchAngle && pViewPunchAngle)
					{
						vPunchAngle = *pPunchAngle;
						pPunchAngle->Init2();
						vViewPunchAngle = *pViewPunchAngle;
						pViewPunchAngle->Init2();
					}
				}


				//--- Thirdperson Deadflag Stuff ---//
				if (in_tp)
				{
					SDK::CBaseAnimState* animstate = local_player->GetAnimState();

					if (!animstate)
						return;

					if (animstate->m_bInHitGroundAnimation && ground_tick > 1)
						*(Vector*)((DWORD)local_player + 0x31C8) = Vector(0, GLOBAL::real_angles.y, 0.f);
					else
						*(Vector*)((DWORD)local_player + 0x31C8) = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0.f);
				}
			}
			break;
		case FRAME_NET_UPDATE_START:
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				if (SETTINGS::settings.beam_bool)
					visuals->DrawBulletBeams();
			}

			break;
		case FRAME_NET_UPDATE_END:

			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
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

					backtracking->DisableInterpolation(entity);
				}
			}

			break;
		}

		original_frame_stage_notify(ecx, stage);

		if (pPunchAngle && pViewPunchAngle)
		{
			*pPunchAngle = vPunchAngle;
			*pViewPunchAngle = vViewPunchAngle;
		}
	}

	bool _fastcall hkDoPostScreenSpaceEffects(void* ecx, void* edx, SDK::CViewSetup* pSetup)
	{
		static auto ofunc = override_view_hook_manager.GetOriginalFunction<do_post_screen_space_effects_t>(44);

		/*if (SETTINGS::settings.glow)
		{
			auto m_local = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
			for (auto i = 0; i < INTERFACES::g_GlowObjManager->size; i++) {
				auto glow_object = &INTERFACES::g_GlowObjManager->m_GlowObjectDefinitions[i];

				SDK::CBaseEntity *m_entity = glow_object->m_pEntity;

				if (!glow_object->m_pEntity || glow_object->IsUnused())
					continue;

				float Red = SETTINGS::settings.glowc.RGBA[0];
				float Green = SETTINGS::settings.glowc.RGBA[1];
				float Blue = SETTINGS::settings.glowc.RGBA[2];
				float A = SETTINGS::settings.glowc.RGBA[3];
				if (m_entity->GetClientClass()->m_ClassID == 35) {
					if (m_entity->GetTeam() == m_local->GetTeam() && m_entity != m_local) continue;

					glow_object->m_vGlowColor = Vector(Red / 255, Green / 255, Blue / 255);
					glow_object->m_flGlowAlpha = A / 255;
					glow_object->m_bRenderWhenOccluded = true;
					glow_object->m_bRenderWhenUnoccluded = false;

					if (m_entity == m_local && SETTINGS::settings.localplayer.glow)
					{
						float Red = SETTINGS::settings.localglow.RGBA[0];
						float Green = SETTINGS::settings.localglow.RGBA[1];
						float Blue = SETTINGS::settings.localglow.RGBA[2];
						float A = SETTINGS::settings.localglow.RGBA[3];

						if (SETTINGS::settings.pulseglow) {
							glow_object->m_bPulsatingChams = true;
						}
						glow_object->m_vGlowColor = Vector(Red / 255, Green / 255, Blue / 255);
						glow_object->m_flGlowAlpha = A / 255;
					}
				}
			}
		}*/

		return ofunc(ecx, pSetup);
	}

	void __fastcall HookedDrawModelExecute(void* ecx, void* edx, SDK::IMatRenderContext* context, const SDK::DrawModelState_t& state, const SDK::ModelRenderInfo_t& render_info, matrix3x4_t* matrix)
	{
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			if (SETTINGS::settings.noflash) {
				SDK::IMaterial* Flash = INTERFACES::MaterialSystem->FindMaterial("effects\\flashbang", "ClientEffect textures");
				SDK::IMaterial* FlashWhite = INTERFACES::MaterialSystem->FindMaterial("effects\\flashbang_white", "ClientEffect textures");
				Flash->SetMaterialVarFlag(SDK::MATERIAL_VAR_NO_DRAW, true);
				FlashWhite->SetMaterialVarFlag(SDK::MATERIAL_VAR_NO_DRAW, true);
				original_draw_model_execute(ecx, context, state, render_info, matrix);
			}
			if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
			{
				for (int i = 1; i < 65; i++)
				{
					auto entity = INTERFACES::ClientEntityList->GetClientEntity(i);
					auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

					if (!entity)
						continue;

					if (!local_player)
						continue;

					if (entity && entity->GetIsScoped() && in_tp && entity == local_player)
					{
						INTERFACES::RenderView->SetBlend(0.4);
					}
				}
			}
			
		}

		original_draw_model_execute(ecx, context, state, render_info, matrix);
	}

	static bool menu_open = false;
	static bool d3d_init = false;
	bool PressedKeys[256] = {};
	void OpenMenu()
	{
		static bool is_down = false;
		static bool is_clicked = false;

		if (GetAsyncKeyState(VK_INSERT))
		{
			is_clicked = false;
			is_down = true;
		}
		else if (!GetAsyncKeyState(VK_INSERT) && is_down)
		{
			is_clicked = true;
			is_down = false;
		}
		else
		{
			is_clicked = false;
			is_down = false;
		}

		if (is_clicked)
		{
			menu_open = !menu_open;
			std::string msg = "cl_mouseenable " + std::to_string(!menu_open);
			INTERFACES::Engine->ClientCmd_Unrestricted(msg.c_str(), 0);
		}
	}

	LRESULT __stdcall Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
		case WM_LBUTTONDOWN:
			PressedKeys[VK_LBUTTON] = true;
			break;
		case WM_LBUTTONUP:
			PressedKeys[VK_LBUTTON] = false;
			break;
		case WM_RBUTTONDOWN:
			PressedKeys[VK_RBUTTON] = true;
			break;
		case WM_RBUTTONUP:
			PressedKeys[VK_RBUTTON] = false;
			break;
		case WM_MBUTTONDOWN:
			PressedKeys[VK_MBUTTON] = true;
			break;
		case WM_MBUTTONUP:
			PressedKeys[VK_MBUTTON] = false;
			break;
		case WM_XBUTTONDOWN:
		{
			UINT button = GET_XBUTTON_WPARAM(wParam);
			if (button == XBUTTON1)
			{
				PressedKeys[VK_XBUTTON1] = true;
			}
			else if (button == XBUTTON2)
			{
				PressedKeys[VK_XBUTTON2] = true;
			}
			break;
		}
		case WM_XBUTTONUP:
		{
			UINT button = GET_XBUTTON_WPARAM(wParam);
			if (button == XBUTTON1)
			{
				PressedKeys[VK_XBUTTON1] = false;
			}
			else if (button == XBUTTON2)
			{
				PressedKeys[VK_XBUTTON2] = false;
			}
			break;
		}
		case WM_KEYDOWN:
			PressedKeys[wParam] = true;
			break;
		case WM_KEYUP:
			PressedKeys[wParam] = false;
			break;
		default: break;
		}

		OpenMenu();

		if (d3d_init && menu_open && ImGui_ImplDX9_WndProcHandler(hWnd, uMsg, wParam, lParam))
			return true;

		return CallWindowProc(INIT::OldWindow, hWnd, uMsg, wParam, lParam);
	}

	void GUI_Init(IDirect3DDevice9* pDevice)
	{
		ImGui_ImplDX9_Init(INIT::Window, pDevice);


		ImGuiIO& io = ImGui::GetIO();


		ImGuiStyle &style = ImGui::GetStyle();

		ImColor mainColor = ImColor(54, 54, 54, 255);
		ImColor bodyColor = ImColor(54, 54, 54, 255);
		ImColor fontColor = ImColor(255, 255, 255, 255);

		ImVec4 mainColorHovered = ImVec4(mainColor.Value.x + 0.1f, mainColor.Value.y + 0.1f, mainColor.Value.z + 0.1f, mainColor.Value.w);
		ImVec4 mainColorActive = ImVec4(mainColor.Value.x + 0.2f, mainColor.Value.y + 0.2f, mainColor.Value.z + 0.2f, mainColor.Value.w);
		ImVec4 menubarColor = ImVec4(bodyColor.Value.x, bodyColor.Value.y, bodyColor.Value.z, bodyColor.Value.w - 0.8f);
		ImVec4 frameBgColor = ImVec4(bodyColor.Value.x, bodyColor.Value.y, bodyColor.Value.z, bodyColor.Value.w + .1f);
		ImVec4 tooltipBgColor = ImVec4(bodyColor.Value.x, bodyColor.Value.y, bodyColor.Value.z, bodyColor.Value.w + .05f);

		style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); //white
		style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.85f, 0.85f, 0.85f, 0.85f); //main quarter
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.70f); //main quarter
		style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 0.60f); //main bg
		style.Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.16f, 0.19f, 0.60f); //main border
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); //dark
		style.Colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); //main bg
		style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.00f, 0.30f, 0.10f, 0.50f); //main colored
		style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); //main bg
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.24f, 0.28f, 0.75f); //collapsed
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); //main bg
		style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 0.70f); //main bg
		style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); //main bg
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); //main half
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.70f); //main half
		style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_ComboBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f); //main bg
		style.Colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); //main half
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); //main
		style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); //main
		style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_Header] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); //main
		style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); //main
		style.Colors[ImGuiCol_Column] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f); //main border
		style.Colors[ImGuiCol_ColumnHovered] = ImVec4(1.00f, 0.30f, 0.10f, 0.50f); //main colored
		style.Colors[ImGuiCol_ColumnActive] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.90f, 0.90f, 0.90f, 0.75f); //main white
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); //main white
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f); //main white
		style.Colors[ImGuiCol_CloseButton] = ImVec4(0.86f, 0.93f, 0.89f, 0.00f); //dark
		style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.86f, 0.93f, 0.89f, 0.40f); //close button
		style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.86f, 0.93f, 0.89f, 0.90f); //close button
		style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.30f, 0.10f, 0.50f); //main colored
		style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.30f, 0.10f, 0.50f); //main colored
		style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 0.30f, 0.10f, 1.00f); //main colored
		style.Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.07f, 0.70f); //main bg
		style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.24f, 0.28f, 0.60f); //collapsed

		style.Alpha = 1.f;
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.FramePadding = ImVec2(1, 1);
		style.ScrollbarSize = 10.f;
		style.ScrollbarRounding = 0.f;
		style.GrabMinSize = 5.f;

		d3d_init = true;
	}

	long __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
	{
		D3DCOLOR rectColor = D3DCOLOR_XRGB(255, 0, 0);
		D3DRECT BarRect = { 1, 1, 1, 1 };

		pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, rectColor, 0, 0);

		if (!d3d_init)
			GUI_Init(pDevice);

		ImGui::GetIO().MouseDrawCursor = menu_open;

		ImGui_ImplDX9_NewFrame();

		POINT mp;

		GetCursorPos(&mp);

		ImGuiIO& io = ImGui::GetIO();

		io.MousePos.x = mp.x;
		io.MousePos.y = mp.y;

		if (menu_open)
		{
			ImGui::SetNextWindowSize(ImVec2(560, 550));
			ImGui::Begin("getmem.es", &menu_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
			{
				DrawMenu();
			}
			ImGui::End();
		}
		ImGui::Render();

		return oEndScene(pDevice);
	}

	long __stdcall Hooked_EndScene_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		if (!d3d_init)
			return oEndSceneReset(pDevice, pPresentationParameters);

		ImGui_ImplDX9_InvalidateDeviceObjects();

		auto hr = oEndSceneReset(pDevice, pPresentationParameters);

		ImGui_ImplDX9_CreateDeviceObjects();

		return hr;
	}

	void __fastcall HookedSceneEnd(void* ecx, void* edx)
	{
		original_scene_end(ecx);


		static SDK::IMaterial* ignorez;
		static SDK::IMaterial* notignorez;

		std::ofstream("csgo\\materials\\simple_ignorez.vmt") << R"#("VertexLitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "1"
  "$envmap"       ""
"$phong"   "1"
  "$phongboost"   "5"
  "$phongexponent"    "25"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";
		std::ofstream("csgo\\materials\\simple_flat.vmt") << R"#("VertexLitGeneric"
{
  "$basetexture" "vgui/white_additive"
  "$ignorez"      "0"
  "$envmap"       ""
"$phong"   "1"
  "$phongboost"   "5"
  "$phongexponent"    "25"
  "$nofog"        "1"
  "$model"        "1"
  "$nocull"       "0"
  "$selfillum"    "1"
  "$halflambert"  "1"
  "$znearer"      "0"
  "$flat"         "1"
}
)#";


			notignorez = INTERFACES::MaterialSystem->FindMaterial("simple_regular", TEXTURE_GROUP_MODEL);
			ignorez = INTERFACES::MaterialSystem->FindMaterial("simple_ignorez", TEXTURE_GROUP_MODEL);
		

		
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			if (SETTINGS::settings.glow)
			{
				auto m_local = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
				for (auto i = 0; i < INTERFACES::g_GlowObjManager->size; i++)
				{
					auto glow_object = &INTERFACES::g_GlowObjManager->m_GlowObjectDefinitions[i];


					SDK::CBaseEntity *m_entity = glow_object->m_pEntity;

					if (!glow_object->m_pEntity || glow_object->IsUnused())
						continue;

					if (m_entity->GetClientClass()->m_ClassID == 35)
					{
						if (m_entity->GetTeam() == m_local->GetTeam() && m_entity != m_local)
							continue;


						bool *m_teammate = new bool;
						*m_teammate = m_entity->GetTeam() == m_local->GetTeam();

						float Red = SETTINGS::settings.glowc[0];
						float Green = SETTINGS::settings.glowc[1];
						float Blue = SETTINGS::settings.glowc[2];
						float A = SETTINGS::settings.glowc[3];

						glow_object->m_vGlowColor = Vector(SETTINGS::settings.glowc[0], SETTINGS::settings.glowc[1], SETTINGS::settings.glowc[2]);
						float val = SETTINGS::settings.glowc[3];
						glow_object->m_flGlowAlpha = val;

						if (m_entity == m_local && SETTINGS::settings.localplayer.glow)
						{
							float Red = SETTINGS::settings.localglow[0];
							float Green = SETTINGS::settings.localglow[1];
							float Blue = SETTINGS::settings.localglow[2];
							float A = SETTINGS::settings.localglow[3];

							if (SETTINGS::settings.pulseglow) {
								glow_object->m_bPulsatingChams = true;
							}
							glow_object->m_vGlowColor = Vector(Red, Green, Blue);
							glow_object->m_flGlowAlpha = A;
						}
						glow_object->m_bRenderWhenOccluded = true;
						glow_object->m_bRenderWhenUnoccluded = false;


						delete m_teammate;
					}
				}
			}

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

				//--- Colored Models ---//

				if (entity && SETTINGS::settings.chams_type == 2)
				{
					ignorez->ColorModulate(SETTINGS::settings.imodel_col); //255, 40, 200
					INTERFACES::ModelRender->ForcedMaterialOverride(ignorez);
					entity->DrawModel(0x1, 255);
					notignorez->ColorModulate(SETTINGS::settings.vmodel_col); //0, 125, 255
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
					entity->DrawModel(0x1, 255);
					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
				else if (entity && SETTINGS::settings.chams_type == 1)
				{
					notignorez->ColorModulate(SETTINGS::settings.vmodel_col); //255, 40, 200
					INTERFACES::ModelRender->ForcedMaterialOverride(notignorez);
					entity->DrawModel(0x1, 255);
					INTERFACES::ModelRender->ForcedMaterialOverride(nullptr);
				}
			}

			//--- Wireframe Smoke ---//
			std::vector<const char*> vistasmoke_wireframe =
			{
				"particle/vistasmokev1/vistasmokev1_smokegrenade",
			};

			std::vector<const char*> vistasmoke_nodraw =
			{
				"particle/vistasmokev1/vistasmokev1_fire",
				"particle/vistasmokev1/vistasmokev1_emods",
				"particle/vistasmokev1/vistasmokev1_emods_impactdust",
			};

			for (auto mat_s : vistasmoke_wireframe)
			{
				SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_s, TEXTURE_GROUP_OTHER);
				mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_WIREFRAME, true); //wireframe
			}

			for (auto mat_n : vistasmoke_nodraw)
			{
				SDK::IMaterial* mat = INTERFACES::MaterialSystem->FindMaterial(mat_n, TEXTURE_GROUP_OTHER);
				mat->SetMaterialVarFlag(SDK::MATERIAL_VAR_NO_DRAW, true);
			}

			static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);
			*(int*)(smokecout) = 0;
		}
	}
	void __fastcall HookedOverrideView(void* ecx, void* edx, SDK::CViewSetup* pSetup)
	{
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());

		if (!local_player)
			return;

		if (GetAsyncKeyState(SETTINGS::settings.thirdkey) & 1)
			in_tp = !in_tp;

		//--- Actual Thirdperson Stuff ---//
		if (INTERFACES::Engine->IsConnected() && INTERFACES::Engine->IsInGame())
		{
			//auto svcheats = INTERFACES::cvar->FindVar("sv_cheats");
			//auto svcheatsspoof = new SDK::SpoofedConvar(svcheats);
			//svcheatsspoof->SetInt(1);

			auto GetCorrectDistance = [&local_player](float ideal_distance) -> float
			{
				Vector inverse_angles;
				INTERFACES::Engine->GetViewAngles(inverse_angles);

				inverse_angles.x *= -1.f, inverse_angles.y += 180.f;

				Vector direction;
				MATH::AngleVectors(inverse_angles, &direction);

				SDK::CTraceWorldOnly filter;
				SDK::trace_t trace;
				SDK::Ray_t ray;

				ray.Init(local_player->GetVecOrigin() + local_player->GetViewOffset(), (local_player->GetVecOrigin() + local_player->GetViewOffset()) + (direction * (ideal_distance + 5.f)));
				INTERFACES::Trace->TraceRay(ray, MASK_ALL, &filter, &trace);

				return ideal_distance * trace.flFraction;
			};

			if (SETTINGS::settings.tp_bool && in_tp)
			{
				if (local_player->GetHealth() <= 0)
					local_player->SetObserverMode(5);

				if (!INTERFACES::Input->m_fCameraInThirdPerson)
				{
					INTERFACES::Input->m_fCameraInThirdPerson = true;

					SDK::CBaseAnimState* animstate = local_player->GetAnimState();

					if (!animstate)
						return;

					if (animstate->m_bInHitGroundAnimation && ground_tick > 1) {
						INTERFACES::Input->m_vecCameraOffset = Vector(0, GLOBAL::real_angles.y, GetCorrectDistance(100));
					}
					else {
						INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, GetCorrectDistance(100));
					}
					Vector camForward;

					MATH::AngleVectors(Vector(INTERFACES::Input->m_vecCameraOffset.x, INTERFACES::Input->m_vecCameraOffset.y, 0), &camForward);
				}
			}
			else
			{
				INTERFACES::Input->m_fCameraInThirdPerson = false;
				INTERFACES::Input->m_vecCameraOffset = Vector(GLOBAL::real_angles.x, GLOBAL::real_angles.y, 0);
			}

			if (!local_player->GetIsScoped())
			{
				if (SETTINGS::settings.fov) {
					pSetup->fov = 90 + SETTINGS::settings.FOV;
				}
				else
					pSetup->fov = 90;
			}
			else
			{
				if (SETTINGS::settings.staticscope)
				{
					if (SETTINGS::settings.fov) {
						pSetup->fov = 90 + SETTINGS::settings.FOV;
					}
					else
						pSetup->fov = 90;
				}
				else
				{

				}
			}
		}
		original_override_view(ecx, pSetup);
	}
	void __fastcall HookedTraceRay(void *thisptr, void*, const SDK::Ray_t &ray, unsigned int fMask, SDK::ITraceFilter *pTraceFilter, SDK::trace_t *pTrace)
	{
		original_trace_ray(thisptr, ray, fMask, pTraceFilter, pTrace);
		pTrace->surface.flags |= SURF_SKY;
	}
	void __fastcall HookedSendDatagram(void* ecx, void* data)
	{
		original_send_datagram(ecx, data);
	}
	bool __fastcall HookedGetBool(void* pConVar, void* edx)
	{
		if ((uintptr_t)_ReturnAddress() == CAM_THINK)
			return true;

		return original_get_bool(pConVar);
	}
	void InitHooks()
	{
		iclient_hook_manager.Init(INTERFACES::Client);
		original_frame_stage_notify = reinterpret_cast<FrameStageNotifyFn>(
			iclient_hook_manager.HookFunction<FrameStageNotifyFn>(36, HookedFrameStageNotify));

		panel_hook_manager.Init(INTERFACES::Panel);
		original_paint_traverse = reinterpret_cast<PaintTraverseFn>(
			panel_hook_manager.HookFunction<PaintTraverseFn>(41, HookedPaintTraverse));

		model_render_hook_manager.Init(INTERFACES::ModelRender);
		original_draw_model_execute = reinterpret_cast<DrawModelExecuteFn>(model_render_hook_manager.HookFunction<DrawModelExecuteFn>(21, HookedDrawModelExecute));

		render_view_hook_manager.Init(INTERFACES::RenderView);
		original_scene_end = reinterpret_cast<SceneEndFn>(render_view_hook_manager.HookFunction<SceneEndFn>(9, HookedSceneEnd));

		override_view_hook_manager.Init(INTERFACES::ClientMode);
		original_override_view = reinterpret_cast<OverrideViewFn>(override_view_hook_manager.HookFunction<OverrideViewFn>(18, HookedOverrideView));
		original_create_move = reinterpret_cast<CreateMoveFn>(override_view_hook_manager.HookFunction<CreateMoveFn>(24, HookedCreateMove));

		trace_hook_manager.Init(INTERFACES::Trace);
		original_trace_ray = reinterpret_cast<TraceRayFn>(trace_hook_manager.HookFunction<TraceRayFn>(5, HookedTraceRay));

		auto sv_cheats = INTERFACES::cvar->FindVar("sv_cheats");
		get_bool_manager = VMT::VMTHookManager(reinterpret_cast<DWORD**>(sv_cheats));
		original_get_bool = reinterpret_cast<SvCheatsGetBoolFn>(get_bool_manager.HookFunction<SvCheatsGetBoolFn>(13, HookedGetBool));

		while (!(INIT::Window = FindWindowA("Valve001", nullptr)))
			Sleep(100);
		if (INIT::Window)
			INIT::OldWindow = (WNDPROC)SetWindowLongPtr(INIT::Window, GWL_WNDPROC, (LONG_PTR)Hooked_WndProc);

		DWORD DeviceStructureAddress = **(DWORD**)(UTILS::FindSignature("shaderapidx9.dll", "A1 ?? ?? ?? ?? 50 8B 08 FF 51 0C") + 1);
		if (DeviceStructureAddress) {
			direct.Init((DWORD**)DeviceStructureAddress);
			oEndSceneReset = reinterpret_cast<EndSceneResetFn>(direct.HookFunction<EndSceneResetFn>(16, Hooked_EndScene_Reset));
			oEndScene = reinterpret_cast<EndSceneFn>(direct.HookFunction<EndSceneFn>(42, Hooked_EndScene));
		}
	}

	void EyeAnglesPitchHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;

	}
	void EyeAnglesYawHook(const SDK::CRecvProxyData *pData, void *pStruct, void *pOut)
	{
		*reinterpret_cast<float*>(pOut) = pData->m_Value.m_Float;

		auto entity = reinterpret_cast<SDK::CBaseEntity*>(pStruct);
		if (!entity)
			return;

		resolver->record(entity, pData->m_Value.m_Float);
	}

	void InitNetvarHooks()
	{
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[0]", EyeAnglesPitchHook);
		UTILS::netvar_hook_manager.Hook("DT_CSPlayer", "m_angEyeAngles[1]", EyeAnglesYawHook);
	}
}
