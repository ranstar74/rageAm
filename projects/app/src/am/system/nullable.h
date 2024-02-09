//
// File: nullable.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <utility>
#include <memory>
#include "asserts.h"

namespace rageam
{
	/**
	 * \brief CSharp-like nullable with HasValue() and GetValue().
	 * \n In short-term it allows to tread stack variables like pointers.
	 * \n Use example:
	 * \n Nullable<atString> GetName() { return Nullable<atString>::Null() }; // We've got no name
	 * \n // ...
	 * \n Nullable<atString> name = GetName();
	 * \n if(name.HasValue())
	 * \n {
	 * \n     atString& entityName = name.GetValue();
	 * \n     // ...
	 * \n }
	 * So as you can see it's similar to having null pointer, but for stack variables.
	 */
	template<typename T>
	class Nullable
	{
		// We use union hack to prevent compiler from generating constructor / destructor,
		// we'll have to control object life-cycle manually
		union Holder
		{
			struct Dummy {}; // NOLINT(clang-diagnostic-microsoft-anon-tag)
			// ReSharper disable once CppUninitializedNonStaticDataMember
			T Value;

			Holder() { memset(this, 0, sizeof Holder); }
			~Holder() { /* Destroy called by nullable only if there's value */ }

			void Destroy()
			{
				Value.~T();
			}
		};

		Holder	m_Holder;
		bool	m_HasValue = false;
	public:
		Nullable() {}
		~Nullable()
		{
			if (m_HasValue)
				m_Holder.Destroy();
		}

		Nullable(const T& value)
		{
			new (&m_Holder.Value) T(value);
			m_HasValue = true;
		}

		Nullable(T&& value)
		{
			new (&m_Holder.Value) T(std::move(value));
			m_HasValue = true;
		}

		Nullable(const Nullable& other)
		{
			if (other.m_HasValue)
			{
				new (&m_Holder.Value) T(other.m_Holder.Value);
				m_HasValue = true;
			}
		}

		T& GetValue()
		{
			AM_ASSERT(m_HasValue, "Nullable::GetValue() -> Has no value! Use HasValue().");
			return m_Holder.Value;
		}

		const T& GetValue() const
		{
			AM_ASSERT(m_HasValue, "Nullable::GetValue() -> Has no value! Use HasValue().");
			return m_Holder.Value;
		}

		void Reset() { m_HasValue = false; }

		/**
		 * \brief You must check if there's value before accessing it!
		 */
		bool HasValue() const { return m_HasValue; }

		/**
		 * \brief You can use this function to reset current value, for example:
		 * \n Nullable<atString> name = atString("rageAm");
		 * \n name = Nullable<atString>::Null();
		 * \n name.HasValue() -> false
		 */
		static Nullable<T> Null() { return Nullable<T>(); }

		Nullable& operator=(const T& value)
		{
			if(m_HasValue)
				m_Holder.Destroy();
			new (&m_Holder.Value) T(value);
			m_HasValue = true;
			return *this;
		}

		Nullable& operator=(const Nullable& other)
		{
			if (m_HasValue)
				m_Holder.Destroy();
			if (other.HasValue())
			{
				new (&m_Holder.Value) T(other.GetValue());
				m_HasValue = true;
			}
			return *this;
		}

		bool operator==(const Nullable& other) const
		{
			if (!m_HasValue && !other.m_HasValue)
				return true;

			if (m_HasValue != other.m_HasValue)
				return false;

			T& left = (T&)m_Holder.Value;
			T& right = (T&)other.m_Holder.Value;
			return left == right;
		}
	};
}
