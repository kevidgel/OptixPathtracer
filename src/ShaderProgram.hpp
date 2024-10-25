//
// Created by kevidgel on 10/22/24.
//

#ifndef SHADERPROGRAM_HPP
#define SHADERPROGRAM_HPP

#include <glad/glad.h>
#include <string>

class ShaderProgram {
public:
  // Program ID
  GLuint program_id;

  ShaderProgram(const char* vertex_shader_source, const char* fragment_shader_source);
  ~ShaderProgram();

  void use();

  void set_bool(const std::string& name, bool value) const;
  void set_int(const std::string& name, int value) const;
  void set_float(const std::string& name, float value) const;
};



#endif //SHADERPROGRAM_HPP
