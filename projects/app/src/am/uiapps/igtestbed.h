#pragma once

#include "am/ui/app.h"
#include "am/ui/extensions.h"
#include "am/ui/imglue.h"
#include "am/ui/font_icons/icons_am.h"
#include "helpers/utf8.h"

#include "am/integration/script/core.h"
#include "am/integration/script/commands.h"

namespace rageam::ui
{
	// Integration testbed
	class IGTestbed : public App
	{
	public:
		// TODO: Init & Shutdown calls must be done outside! In integration

		IGTestbed()
		{

		}

		~IGTestbed()
		{

		}

		void OnRender() override
		{
			ImGui::Begin("IG Testbed");

#ifdef AM_INTEGRATED

			if (ImGui::Button("scrSqrt"))
			{
				scrBegin();

				float f = scrSqrt(25);
				AM_TRACEF("SQRT 25 -> %f", f);

				scrEnd();
			}

			static scrObjectIndex object;

			if (ImGui::Button("scrCreateObject"))
			{
				scrBegin();

				if (object)
					scrDeleteObject(object);
				object = scrCreateObject(Hash("prop_jetski_ramp_01"), scrVector(-1563, -1084, 6));

				AM_TRACEF("Object Index - %i", object.Get());

				scrEnd();
			}

			if (ImGui::Button("scrDeleteObject"))
			{
				scrBegin();

				scrDeleteObject(object);
				AM_TRACEF("Object Index - %i", object.Get());

				scrEnd();
			}

			if (ImGui::Button("scrRequestScript"))
			{
				scrBegin();
				scrRequestScript("audiotest");
				scrEnd();
			}

			if (ImGui::Button("Wrap Ped"))
			{
				//scrDispatch([]
				//	{
				//		scrVector pos(-1560, -1080, 10);
				//		float groundZ;
				//		scrGetGroundZFor3DCoord(pos, groundZ);
				//		pos.Z = groundZ + 4.0f; // Add player height
				//		scrSetPedCoordsKeepVehicle(scrPlayerGetID(), pos);
				//	});
				scrVector pos(-1560, -1080, 10);
				float groundZ;
				scrGetGroundZFor3DCoord(pos, groundZ);
				pos.Z = groundZ + 4.0f; // Add player height
				scrSetPedCoordsKeepVehicle(scrPlayerGetID(), pos);
			}

			//if(ImGui::Button("Test"))
			//{
			//	scrBegin_();

			//	static auto fn1 = gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? 33 C9 48 85 C0 74 0C")
			//		.ToFunc<pVoid()>();

			//	static auto fn2 = gmAddress::Scan("8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 BA 48")
			//		.ToFunc<pVoid()>();

			//	pVoid crap1 = fn1();
			//	pVoid crap2 = fn2();

			//	scrEnd_();

			//	AM_TRACEF("");
			//}

#endif

			ImGui::End();
		}
	};
}
