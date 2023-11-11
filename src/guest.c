#include "guest.h"
#include "game.h"

ESKE_EXPORT bool guest(guest_params_t params, guest_cmd_t cmd)
{
  game_state_t *s = (game_state_t*)params.hunk;
  s->hunk = params.hunk;

  switch (cmd)
  {
  case GUEST_CMD_INIT: game_init(s); break;
  case GUEST_CMD_TICK: return game_tick(s);
  // todo: find something to do here?
  case GUEST_CMD_UNLOAD: game_hot_unload(s); break;
  case GUEST_CMD_RELOAD: game_hot_reload(s); break;
  }
  return true;
}
