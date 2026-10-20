#pragma once

#include <QElapsedTimer>
#include <QtGlobal>
#include <cstdio>

namespace StartupProfile {

inline QElapsedTimer& timer()
{
    static QElapsedTimer t = [] {
        QElapsedTimer x;
        x.start();
        return x;
    }();
    return t;
}

inline const int started = (timer(), 0);

inline bool enabled()
{
    static const bool e = qEnvironmentVariableIntValue("BEE_STARTUP_PROFILE") != 0;
    return e;
}

inline void mark(const char* what)
{
    if (enabled())
        std::fprintf(stderr, "[profile] %s: %lld ms\n", what, static_cast<long long>(timer().elapsed()));
}

}  // namespace StartupProfile
