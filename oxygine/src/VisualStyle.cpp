#include "VisualStyle.h"
#include "Actor.h"
#include <sstream>
#include "RenderState.h"

namespace oxygine
{
	void VisualStyle::_apply(const RenderState &rs)
	{
		rs.renderer->setBlendMode(_blend);

		Color color = _color;
		color.a = (color.a * rs.alpha) / 255;
		rs.renderer->setPrimaryColor(color);
	}

	string VisualStyle::dump() const
	{
		VisualStyle def;

		stringstream stream;
		if (_color != def.getColor())
		{
			stream << "color=(" << (int)_color.r << ", " << (int)_color.g << ", " << (int)_color.b << ", " << (int)_color.a << ")";
		}

		if (_blend != def.getBlendMode())
		{
			stream << "blend=" << (int)_blend;
		}
		

		return stream.str();
	}
}