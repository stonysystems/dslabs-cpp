
#include "../deptran/__dep__.h"
#include "client.h"
#include "server.h"

namespace janus {
KvClient* cli;

void SimpleTest() {
  int ret = cli->Append("test", "hello world");
  verify(ret == KV_SUCCESS); 
  string v;
  ret = cli->Get("test", &v);
  verify(v == "hello world"); 
}

} // namespace janus;



