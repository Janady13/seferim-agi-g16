#include <napi.h>
#include "abi.h"
using namespace Napi;

Value UpdateWrapped(const CallbackInfo& info) {
  Env env = info.Env();
  if (info.Length() != 3 || !info[0].IsArray() || !info[1].IsArray() || !info[2].IsObject())
    return env.Null();
  auto s_t_arr = info[0].As<Array>();
  auto s_tm1_arr = info[1].As<Array>();
  auto in_obj = info[2].As<Object>();
  g16_meta_state_c s_t{}, s_tm1{}, out{};
  for (uint32_t i = 0; i < 17 && i < s_t_arr.Length(); ++i) s_t.g[i] = s_t_arr.Get(i).As<Number>().DoubleValue();
  for (uint32_t i = 0; i < 17 && i < s_tm1_arr.Length(); ++i) s_tm1.g[i] = s_tm1_arr.Get(i).As<Number>().DoubleValue();
  g16_inputs_c in{};
  in.dx_norm = in_obj.Get("dx_norm").As<Number>().DoubleValue();
  in.ed_error = in_obj.Get("ed_error").As<Number>().DoubleValue();
  in.utility = in_obj.Get("utility").As<Number>().DoubleValue();
  in.stability = in_obj.Get("stability").As<Number>().DoubleValue();
  g16_update(&s_t, &s_tm1, &in, &out);
  Array res = Array::New(env, 17);
  for (uint32_t i = 0; i < 17; ++i) res.Set(i, Number::New(env, out.g[i]));
  return res;
}

Object Init(Env env, Object exports) {
  exports.Set("update", Function::New(env, UpdateWrapped));
  return exports;
}

NODE_API_MODULE(g16addon, Init)

