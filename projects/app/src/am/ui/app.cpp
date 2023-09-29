#include "app.h"

#include "context.h"

rageam::InputState& rageam::ui::App::GetInput() const
{
	return Gui->Input.State;
}
