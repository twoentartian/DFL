#pragma once

#define GENERATE_GET(Var_name, get_name)    \
auto& get_name()                            \
{return Var_name;}                          \
[[nodiscard]] auto& get_name() const        \
{return Var_name;}
