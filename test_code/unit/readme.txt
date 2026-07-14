These exercise individual functions with all external calls mocked.

prodrv_main.c
  init_ifnet()
  init_context()
  init_protostatics()  
  init_protomanstatics()
  proto_shutdown()
  validate_source_socket()
  dispatch_message()

prodrv_handlers.c
  handle_MSG_INIT_PRODRV()
  send_MSG_ACK_init_prodrv()
  handle_MSG_PREPARE_FRAME()
  handle_MSG_READ_CSH_CHUNK()
  handle_MSG_PROTOMAN_CONTROL()