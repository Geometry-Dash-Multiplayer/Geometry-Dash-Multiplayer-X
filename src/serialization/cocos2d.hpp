#include <Geode/cocos/include/cocos2d.h>

namespace boost::serialization
{
  void serialize(auto& arch, cocos2d::CCPoint& point,
                 const unsigned int version)
  {
    arch & point.x & point.y;
  }
} // namespace boost::serialization
