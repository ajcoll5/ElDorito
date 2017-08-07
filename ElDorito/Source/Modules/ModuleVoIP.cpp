#include "ModuleVoIP.hpp"
#include <sstream>
#include "../Patches/Core.hpp"
#include "../Patches/Ui.hpp"
#include "../Patches/Input.hpp"
#include "../Web/Ui/ScreenLayer.hpp"
#include "../ThirdParty/rapidjson/writer.h"
#include "../ThirdParty/rapidjson/stringbuffer.h"

namespace
{
	static bool ready = false;
	static bool isMainMenu = false;
	static bool isChatting = false;

	bool UpdateVoip(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (!ready)
			return false;

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();
		writer.Key("PTT_Enabled");
		writer.Int(Modules::ModuleVoIP::Instance().VarPTTEnabled->ValueInt);
		writer.Key("MicrophoneID");
		writer.String(Modules::ModuleVoIP::Instance().VarMicrophone->ValueString.c_str());
		writer.Key("EchoCancellation");
		writer.Int(Modules::ModuleVoIP::Instance().VarEchoCancellation->ValueInt);
		writer.Key("AGC");
		writer.Int(Modules::ModuleVoIP::Instance().VarAGC->ValueInt);
		writer.Key("NoiseSupress");
		writer.Int(Modules::ModuleVoIP::Instance().VarNoiseSupress->ValueInt);
		writer.Key("Enabled");
		writer.Int(Modules::ModuleVoIP::Instance().VarVoipEnabled->ValueInt);
		writer.EndObject();

		Web::Ui::ScreenLayer::Notify("voip-settings", buffer.GetString(), true);

		if (Modules::ModuleVoIP::Instance().VarPTTEnabled->ValueInt == 0)
			Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::Available);
		else
			Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::PushToTalk);

		returnInfo = "";
		return true;
	}

	void RendererStarted(const char* map)
	{
		if (std::string(map).find("mainmenu") == std::string::npos)
		{
			isMainMenu = false;

			//initial voip icons when maps loads
			if (Modules::ModuleVoIP::Instance().VarPTTEnabled->ValueInt == 0)
				Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::Available);
			else
				Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::PushToTalk);
		}
		else
			isMainMenu = true;
		ready = true;
	}

	void OnGameInputUpdated()
	{
		auto isUsingController = *(bool*)0x0244DE98;
		Blam::Input::BindingsTable bindings;
		GetBindings(0, &bindings);
		
		if (Modules::ModuleVoIP::Instance().VarPTTEnabled->ValueInt == 1)
		{
			if (isMainMenu && isUsingController)
				return;
			//keyboard/controller in-game
			if (isChatting && Blam::Input::GetActionState(Blam::Input::eGameActionVoiceChat)->Ticks == 0)
			{
				isChatting = false;
				if (!isMainMenu)
					if (!isMainMenu)
					{
						Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::PushToTalk);

						static auto Sound_PlaySoundEffect = (void(*)(uint32_t sndTagIndex, int a2))(0x5DC6B0);
						//static auto sub_5D7290 = (void(*)(std::string a1, uint32_t sndTagIndex, int a2, int a3))(0x5D7290);

						//Make sure the sound exists before playing
						typedef void *(*GetTagAddressPtr)(int groupTag, uint32_t index);
						auto GetTagAddressImpl = reinterpret_cast<GetTagAddressPtr>(0x503370);
						if (GetTagAddressImpl('lsnd', 0x000015AD) != nullptr)
						{
							int a2 = -1;
							Sound_PlaySoundEffect(0x000015AD, a2);
						}
					}
				Web::Ui::ScreenLayer::Notify("voip-ptt", "{\"talk\":0}", true);
			}
			else if (!isChatting && Blam::Input::GetActionState(Blam::Input::eGameActionVoiceChat)->Ticks == 1)
			{
				isChatting = true;
				if (!isMainMenu)
				{
					Patches::Ui::SetVoiceChatIcon(Patches::Ui::VoiceChatIcon::Speaking);

					static auto Sound_PlaySoundEffect = (void(*)(uint32_t sndTagIndex, int a2, int a3, int a4, char a5))(0x5DC530);
					//static auto sub_5D7290 = (void(*)(std::string a1, uint32_t sndTagIndex, int a2, int a3))(0x5D7290);

					//Make sure the sound exists before playing
					typedef void *(*GetTagAddressPtr)(int groupTag, uint32_t index);
					auto GetTagAddressImpl = reinterpret_cast<GetTagAddressPtr>(0x503370);
					if (GetTagAddressImpl('lsnd', 0x000015AD) != nullptr)
					{
						int a2 = -1;
						int a3 = 1065353216;
						int a4 = 0;
						char a5 = 0;
						Sound_PlaySoundEffect(0x000015AD, a2, a3, a4, a5);
					}
				}

				Web::Ui::ScreenLayer::Notify("voip-ptt", "{\"talk\":1}", true);
			}
		}
	}

	void OnUiInputUpdated()
	{
		auto isUsingController = *(bool*)0x0244DE98;
		Blam::Input::BindingsTable bindings;
		GetBindings(0, &bindings);

		//controller in lobby
		if (isMainMenu && isUsingController && Modules::ModuleVoIP::Instance().VarPTTEnabled->ValueInt == 1)
		{
			if (isChatting && Blam::Input::GetActionState((Blam::Input::GameAction)bindings.ControllerButtons[Blam::Input::eGameActionVoiceChat])->Ticks == 0)
			{
				isChatting = false;
				Web::Ui::ScreenLayer::Notify("voip-ptt", "{\"talk\":0}", true);
			}
			else if (!isChatting && Blam::Input::GetActionState((Blam::Input::GameAction)bindings.ControllerButtons[Blam::Input::eGameActionVoiceChat])->Ticks == 1)
			{
				isChatting = true;
				Web::Ui::ScreenLayer::Notify("voip-ptt", "{\"talk\":1}", true);
			}
		}
	}
}

namespace Modules
{
	ModuleVoIP::ModuleVoIP() : ModuleBase("VoIP")
	{
		VarVoipEnabled = AddVariableInt("Enabled", "enabled", "Toggle voip on or off", eCommandFlagsArchived, 1);
		VarVoipEnabled->ValueIntMin = 0;
		VarVoipEnabled->ValueIntMax = 1;

		VarPTTEnabled = AddVariableInt("PTT_Enabled", "ptt_enabled", "Enable PTT(1) or voice activation(0)", eCommandFlagsArchived, 1);
		VarPTTEnabled->ValueIntMin = 0;
		VarPTTEnabled->ValueIntMax = 1;

		VarMicrophone = AddVariableString("MicrophoneID", "microphoneid", "microphone label to use for voip, blank is default device", eCommandFlagsArchived, "");

		VarEchoCancellation = AddVariableInt("EchoCancelation", "echocancellation", "Toggle echo cancellation", eCommandFlagsArchived, 1);
		VarEchoCancellation->ValueIntMin = 0;
		VarEchoCancellation->ValueIntMax = 1;

		VarAGC = AddVariableInt("AGC", "agc", "Toggle automatic gain control", eCommandFlagsArchived, 1);
		VarAGC->ValueIntMin = 0;
		VarAGC->ValueIntMax = 1;

		VarNoiseSupress = AddVariableInt("NoiseSupress", "noisesupress", "Toggle noise supression", eCommandFlagsArchived, 1);
		VarNoiseSupress->ValueIntMin = 0;
		VarNoiseSupress->ValueIntMax = 1;

		AddCommand("Update", "update", "Updates the voip screen layer with variable states", eCommandFlagsNone, UpdateVoip);


		Patches::Core::OnMapLoaded(RendererStarted);
		Patches::Input::RegisterDefaultInputHandler(OnGameInputUpdated);
		Patches::Input::RegisterMenuUIInputHandler(OnUiInputUpdated);
	}
}