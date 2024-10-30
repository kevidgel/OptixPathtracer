//
// Created by kevidgel on 10/22/24.
//

#pragma once

#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/glad.h>
#include <string>

class Shader {
public:
  // Program ID
  GLuint program_id;

  Shader(const char* vertex_shader_source, const char* fragment_shader_source);
  ~Shader();

  void use();

  void set_bool(const std::string& name, bool value) const;
  void set_int(const std::string& name, int value) const;
  void set_float(const std::string& name, float value) const;
};



#endif //SHADER_HPP
