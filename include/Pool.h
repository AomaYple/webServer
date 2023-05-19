#pragma once

class Pool {
public:
    explicit Pool(unsigned short port, bool stopLog = false);

    Pool(const Pool &threadPool) = delete;

    Pool(Pool &&threadPool) = delete;
};
