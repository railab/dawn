// dawn/include/dawn/proto/shell/pretty.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>
#include <string>

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"

namespace dawn
{
// Forward declaration

class IIOCommon;
class CIOCommon;
struct io_ddata_t;

/**
 * @brief Interactive Command-Line Shell Interface.
 *
 * This class implements a full-featured interactive shell for device
 * management and debugging.
 */

class CProtoShellPretty
  : public CProtoCommon
  , protected CThreadedObject
{
public:
  static int inst;         ///< Global instance counter for multiple shell instances.
  static FILE *outstream_; ///< Global output stream for shell messages.

  enum
  {
    PROTO_SHELL_CFG_FIRST = 0,
    PROTO_SHELL_CFG_IOBIND = 1, ///< I/O object binding configuration.
    PROTO_SHELL_CFG_PATH = 2,   ///< Shell device path (TTY/serial).
    PROTO_SHELL_CFG_PROMPT = 3, ///< Custom prompt string.
    PROTO_SHELL_CFG_LAST = 31
  } typedef EProtoShellCfg;

  enum
  {
    PROTO_SHELL_CMDID_INVAL = 0,
    PROTO_SHELL_CMDID_HELP,
    PROTO_SHELL_CMDID_EXIT,
    PROTO_SHELL_CMDID_INFO,
    PROTO_SHELL_CMDID_GETIO,
#ifdef CONFIG_DAWN_IO_NOTIFY
    PROTO_SHELL_CMDID_GETIONOTIFY,
#endif
    PROTO_SHELL_CMDID_GETIOLOOP,
    PROTO_SHELL_CMDID_SETIO,
    PROTO_SHELL_CMDID_SETCFG,
    PROTO_SHELL_CMDID_GETCFG,
#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
    PROTO_SHELL_CMDID_LIST,
    PROTO_SHELL_CMDID_INSPECT,
    PROTO_SHELL_CMDID_TREE,
    PROTO_SHELL_CMDID_STATS,
#endif
  } typedef EProtoShellCmdId;

  static_assert(PROTO_SHELL_CFG_LAST - 1 <= SObjectCfg::ID_MAX);

  struct
  {
    SObjectId::UObjectId objid; ///< I/O object ID to expose in shell.
  } typedef SProtoShellIOBind;

  explicit CProtoShellPretty(CDescObject &desc)
    : CProtoCommon(desc)
    , instream(nullptr)
    , outstream(nullptr)
  {
  }

  ~CProtoShellPretty() override;

  CProtoShellPretty(const CProtoShellPretty &) = delete;
  CProtoShellPretty &operator=(const CProtoShellPretty &) = delete;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "shell";
  }
#endif

  int configure() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  constexpr static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_SHELL_STD, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_SHELL_STD, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoShellPretty::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_SHELL_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPath(uint16_t size)
  {
    return CProtoShellPretty::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_SHELL_CFG_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPrompt(uint16_t size)
  {
    return CProtoShellPretty::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_SHELL_CFG_PROMPT);
  }

private:
  const char *promptDefault = "dsh> "; ///< Default prompt string.
  const char *prompt = promptDefault;  ///< Custom prompt (nullptr = default).
  const char *path = nullptr;          ///< Shell device path (e.g., /dev/ttyUSB0).
  FILE *instream;                      ///< Input stream for reading user commands.
  FILE *outstream;                     ///< Output stream for shell output.

  std::map<std::string, EProtoShellCmdId> cmdmap = {
    {"help", PROTO_SHELL_CMDID_HELP},
    {"exit", PROTO_SHELL_CMDID_EXIT},
    {"info", PROTO_SHELL_CMDID_INFO},
    {"getio", PROTO_SHELL_CMDID_GETIO},
    {"setio", PROTO_SHELL_CMDID_SETIO},
#ifdef CONFIG_DAWN_IO_NOTIFY
    {"getionotify", PROTO_SHELL_CMDID_GETIONOTIFY},
#endif
    {"getioloop", PROTO_SHELL_CMDID_GETIOLOOP},
    {"setcfg", PROTO_SHELL_CMDID_SETCFG},
    {"getcfg", PROTO_SHELL_CMDID_GETCFG},
#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
    {"list", PROTO_SHELL_CMDID_LIST},
    {"inspect", PROTO_SHELL_CMDID_INSPECT},
    {"tree", PROTO_SHELL_CMDID_TREE},
    {"stats", PROTO_SHELL_CMDID_STATS},
#endif
  };

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int notifierCb(void *priv, io_ddata_t *data);
#endif

  void setPrompt(const char *p);
  void getAndPrint(CIOCommon *io) const;
  void getAndPrintSeek(CIOCommon *io) const;
  void getcfgAndPrint(CObject *io) const;

  void cmdHelp();
  void cmdExit();
  void cmdInfo();
  void cmdGetio(const char *arg);
  void cmdGetioloop(const char *arg);

#ifdef CONFIG_DAWN_IO_NOTIFY
  void cmdGetioNotify(const char *arg);
#endif

  void cmdSetio(const char *arg);
  void cmdSetcfg(const char *arg);
  void cmdGetcfg(const char *arg);

#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
  void cmdList(const char *arg);
  void cmdInspect(const char *arg);
  void cmdTree(const char *arg);
  void cmdStats(const char *arg);
#endif

  void cmdHandle(char *buffer, const char *arg);
  int configureDesc(const CDescObject &desc);
  void allocObject(const SObjectId::UObjectId &obj);
  void thread();
};
} // Namespace dawn
