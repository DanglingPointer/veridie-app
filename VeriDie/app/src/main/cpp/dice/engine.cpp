#include <random>
#include "dice/engine.hpp"

using namespace dice;

namespace {

class UniformEngine : public IEngine
{
public:
   UniformEngine()
      : m_rd()
      , m_generator(m_rd())
   {}
   template <typename D>
   void operator()(std::vector<D> & cast)
   {
      std::uniform_int_distribution<uint32_t> dist(D::MIN, D::MAX);
      for (auto & value : cast) {
         value(dist(m_generator));
      }
   }
   void GenerateResult(Cast & cast) override { std::visit(*this, cast); }

private:
   std::random_device m_rd;
   std::mt19937 m_generator;
};

} // namespace

namespace dice {

std::unique_ptr<IEngine> CreateUniformEngine()
{
   return std::make_unique<UniformEngine>();
}

} // namespace dice
