#include "key_event.h"

namespace sc {

std::string KeyEvent::key_name() const {
    switch (type) {
        case Char: return std::string(1, ch);
        case Special:
        case Modified: return name;
    }
    return "?";
}

} // namespace sc
