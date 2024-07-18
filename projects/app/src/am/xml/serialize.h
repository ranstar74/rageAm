// ReSharper disable CppClangTidyBugproneMacroParentheses
#pragma once

#include "node.h"

/**
 * \brief Interface for data that can be serialized.
 */
struct IXml
{
	virtual ~IXml() = default;

	virtual void Serialize(XmlHandle& node) const = 0;
	virtual void Deserialize(const XmlHandle& node) = 0;

	bool operator==(const IXml&) const = default;
};

// Option to export write only modified values in XML file, makes it look cleaner but harder to edit manually
// #define XML_EXPORT_ONLY_MODIFIED_VALUES

// Defines static instance with default values, we need that to export only modified fields
#define XML_DEFINE(type) static const type& GetDefault() { static type s_Default; return s_Default; } MACRO_END

#define XML_COMPARE(var) GetDefault().var != var

#ifdef XML_EXPORT_ONLY_MODIFIED_VALUES
#define XML_COMPARE_DEFAULT(var) XML_COMPARE(var)
#else
#define XML_COMPARE_DEFAULT(var) true
#endif

// Those macro's exist to easily substitute field name as name string

// Adds attribute with value
// <Node var="var.Value" />
#define XML_SET_ATTR(node, var) if(XML_COMPARE_DEFAULT(var)) node.SetAttribute(#var, var)
// Gets attribute value, ignores if value is not specified
// <Node var="var.Value" />
#define XML_GET_ATTR(node, var) node.GetAttribute(#var, var, true)
// Same as XML_GET_ATTR but throws exception if value is not specified
#define XML_GET_ATTR_R(node, var) node.GetAttribute(#var, var, false)

// Gets child sets value
// <Node>
//     <var>Value<var/>
// </Node>
#define XML_GET_CHILD_VALUE(node, var) node.GetChild(#var).GetValue(var)
// Adds child and sets 'Value' attribute
// <Node>
//     <var Value="var.Value" />
// </Node>
#define XML_SET_CHILD_VALUE_ATTR(node, var) if(XML_COMPARE_DEFAULT(var)) node.AddChild(#var).SetTheValueAttribute(var)
// Gets child's 'Value' attribute, ignores if value is not specified
// <Node>
//     <var Value="var.Value" />
// </Node>
#define XML_GET_CHILD_VALUE_ATTR(node, var) node.GetChild(#var).GetTheValueAttribute(var, true)
// Same as XML_GET_CHILD_VALUE but throws exception if value is not specified
#define XML_GET_CHILD_VALUE_ATTR_R(node, var) node.GetChild(#var).GetTheValueAttribute(var, false)

// ==
// Same as above, but ignore default values
// ==
// Adds child and sets value
// <Node>
//     <var>Value<var/>
// </Node>
#define XML_SET_CHILD_VALUE_IGNORE_DEF(node, var) if(XML_COMPARE(var)) node.AddChild(#var).SetValue(var)
// Adds attribute with value
// <Node var="var.Value" />
#define XML_SET_ATTR_IGNORE_DEF(node, var) if(XML_COMPARE(var)) node.SetAttribute(#var, var)
// Adds child and sets 'Value' attribute
// <Node>
//     <var Value="var.Value" />
// </Node>
#define XML_SET_CHILD_VALUE_ATTR_IGNORE_DEF(node, var) if(XML_COMPARE(var)) node.AddChild(#var).SetTheValueAttribute(var)

#define XML_GET_CHILD_VALUE_FLAGS(node, var, func) if (!node.GetChild(#var).IsNull()) var = func(node.GetChild(#var).GetText())
#define XML_SET_CHILD_VALUE_FLAGS_IGNORE_DEF(node, var, func) if(XML_COMPARE(var)) node.AddChild(#var).SetValue(func(var))
