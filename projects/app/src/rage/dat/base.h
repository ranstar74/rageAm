#pragma once

namespace rage
{
	/**
	 * \brief This class is empty in release game builds and there's not much info on it,
	 * we need it just for virtual function table.
	 */
	class datBase
	{
	public:
		datBase() = default;
		virtual ~datBase() = default;
	};
}
