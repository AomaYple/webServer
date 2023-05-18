#ifndef WEBSERVER_POOL_H
#define WEBSERVER_POOL_H

class Pool {
 public:
  explicit Pool(unsigned short port, bool stopLog = false);

  Pool(const Pool& threadPool) = delete;

  Pool(Pool&& threadPool) = delete;
};

#endif  //WEBSERVER_POOL_H
