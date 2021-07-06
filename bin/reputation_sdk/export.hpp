#pragma once

#define EXPORT_REPUTATION_API                                                               \
extern "C" BOOST_SYMBOL_EXPORT reputation_implementation<float> reputation_float;           \
reputation_implementation<float> reputation_float;                                          \
extern "C" BOOST_SYMBOL_EXPORT reputation_implementation<double> reputation_double;         \
reputation_implementation<double> reputation_double;

constexpr char const* export_class_name_reputation_float = "reputation_float";
constexpr char const* export_class_name_reputation_double = "reputation_double";