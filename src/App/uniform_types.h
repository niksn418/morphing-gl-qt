#pragma once

template <typename T>
struct UniformLocType {
	GLint loc = -1;
};

template <typename T>
static UniformLocType<T> bindUniform(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
									 QString varName)
{
	int index = shaderProgram->uniformLocation(varName);
	if (index == -1)
	{
		qDebug() << "ERROR: could not get shader loc for" << varName << '\n';
	}
	return {index};
}

template <typename T>
static void setUniformValue(std::shared_ptr<QOpenGLShaderProgram> shaderProgram,
							const UniformLocType<T> & locs, const T & value)
{
	shaderProgram->setUniformValue(locs.loc, value);
}

#define __declare_field(tp, name) tp name;
#define __declare_loc_field(tp, name) UniformLocType<tp> name;
#define __assign_loc(tp, name) .name = bindUniform<tp>(shaderProgram, varName + "." #name),
#define __set_uniform(tp, name) setUniformValue<tp>(shaderProgram, locs.name, value.name);

#define define_uniform_struct(name, fields)															\
	struct name {																					\
		fields(__declare_field)																		\
	};																								\
	template <>																						\
	struct UniformLocType<name> {																	\
		fields(__declare_loc_field)																	\
	};																								\
	template <>																						\
	[[maybe_unused]] UniformLocType<name> bindUniform<name>(										\
											std::shared_ptr<QOpenGLShaderProgram> shaderProgram,	\
											QString varName)										\
	{																								\
		return UniformLocType<name>{ fields(__assign_loc) };										\
	}																								\
	template <>																						\
	[[maybe_unused]] void setUniformValue<name>(													\
							std::shared_ptr<QOpenGLShaderProgram> shaderProgram,					\
							const UniformLocType<name> & locs, const name & value)					\
	{																								\
		fields(__set_uniform)																		\
	}
