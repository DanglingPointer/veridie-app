#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "dice/cast.hpp"

namespace dice {

class IEngine
{
public:
   virtual ~IEngine() = default;

   virtual void GenerateResult(dice::Cast & cast) = 0;
};

std::unique_ptr<IEngine> CreateUniformEngine();

} // namespace dice

#endif // ENGINE_HPP
