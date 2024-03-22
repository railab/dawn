// dawn/src/proto/shell/pretty.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/shell/pretty.hxx"
#include "dawn/dev/shutdown.hxx"

#include <chrono>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/prog/common.hxx"
#include "system/readline.h"

#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
#  include "dawn/dev/inspector.hxx"
#endif

using namespace dawn;
using namespace std::chrono_literals;

#define DAWN_PROTO_SHELL_LINELEN 64
#define SHELLPRINT               printf

#define GETANDPRINT_DIM_MAX      128
#define SHELL_SEEK_CHUNK_SIZE    64
#define SHELL_SEEK_LINE_BYTES    16

int CProtoShellPretty::inst = 0;
FILE *CProtoShellPretty::outstream_ = 0;

static bool parseU32Token(const char *token, uint32_t *out)
{
  unsigned long parsed;
  char *endptr = nullptr;

  if (token == nullptr || out == nullptr || *token == '\0')
    {
      return false;
    }

  parsed = strtoul(token, &endptr, 0);
  if (*endptr != '\0' || parsed > UINT32_MAX)
    {
      return false;
    }

  *out = static_cast<uint32_t>(parsed);
  return true;
}

template<typename T>
static void printTypedDataHelper(FILE *outstream,
                                 CIOCommon *io,
                                 size_t ddim,
                                 const char *fmt,
                                 const T *data_ptr,
                                 uint64_t ts)
{
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (io->isTimestamp())
    {
      fprintf(outstream, "  ts:%" PRId64 " ", ts);
    }
#endif

  for (size_t i = 0; i < ddim; i++)
    {
      fprintf(outstream, fmt, data_ptr[i]);
    }
}

#ifdef CONFIG_DAWN_DTYPE_CHAR
static void printCharDataHelper(FILE *outstream,
                                CIOCommon *io,
                                size_t ddim,
                                const char *data_ptr,
                                uint64_t ts)
{
#  ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (io->isTimestamp())
    {
      fprintf(outstream, "  ts:%" PRId64 " ", ts);
    }
#  endif

  for (size_t i = 0; i < ddim; i++)
    {
      if (data_ptr[i] == '\0')
        {
          break;
        }

      fprintf(outstream, "%c", data_ptr[i]);
    }
}
#endif

template<typename T>
static bool getAndPrintTyped(FILE *outstream, CIOCommon *io, size_t ddim, const char *fmt)
{
  io_sdata_t<T, GETANDPRINT_DIM_MAX> data;
  if (data.getItems() < ddim)
    {
      fprintf(outstream, "Data dimension %zu exceeds buffer size %zu\n", ddim, data.getItems());
      return false;
    }

  int ret = io->getData(data, 1);
  if (ret < 0)
    {
      fprintf(outstream, "getio failed %d\n", ret);
      return false;
    }

  printTypedDataHelper<T>(outstream, io, ddim, fmt, &data(0), data[0]);
  return true;
}

#ifdef CONFIG_DAWN_DTYPE_CHAR
static bool getAndPrintChar(FILE *outstream, CIOCommon *io, size_t ddim)
{
  io_sdata_t<char, GETANDPRINT_DIM_MAX> data;
  if (data.getItems() < ddim)
    {
      fprintf(outstream, "Data dimension %zu exceeds buffer size %zu\n", ddim, data.getItems());
      return false;
    }

  int ret = io->getData(data, 1);
  if (ret < 0)
    {
      fprintf(outstream, "getio failed %d\n", ret);
      return false;
    }

  printCharDataHelper(outstream, io, ddim, &data(0), data[0]);
  return true;
}
#endif

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoShellPretty::notifierCb(void *priv, io_ddata_t *data)
{
  CIOCommon *io = (CIOCommon *)priv;

  if (io == nullptr)
    {
      DAWNERR("NULL io pointer in notifier callback\n");
      return -EINVAL;
    }

  size_t ddim = io->getDataDim();
  int dtype = io->getDtype();

  fprintf(outstream_, "IO 0x%" PRIx32 " data:\n\t", io->getIdV());

  uint64_t ts = 0;
#  ifdef CONFIG_DAWN_IO_TIMESTAMP
  ts = (*data)[0];
#  endif
  (void)ddim;
  (void)ts;

  switch (dtype)
    {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        printTypedDataHelper<bool>(outstream_, io, ddim, "%" PRId8 " ", &data->get<bool>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        printTypedDataHelper<int8_t>(
          outstream_, io, ddim, "%" PRId8 " ", &data->get<int8_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        printTypedDataHelper<uint8_t>(
          outstream_, io, ddim, "%" PRId8 " ", &data->get<uint8_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        printTypedDataHelper<int16_t>(
          outstream_, io, ddim, "%" PRId16 " ", &data->get<int16_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        printTypedDataHelper<uint16_t>(
          outstream_, io, ddim, "%" PRId16 " ", &data->get<uint16_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        printTypedDataHelper<int32_t>(
          outstream_, io, ddim, "%" PRId32 " ", &data->get<int32_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        printTypedDataHelper<uint32_t>(
          outstream_, io, ddim, "%" PRId32 " ", &data->get<uint32_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        printTypedDataHelper<uint64_t>(
          outstream_, io, ddim, "%" PRId64 " ", &data->get<uint64_t>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        printTypedDataHelper<float>(outstream_, io, ddim, "%.4f ", &data->get<float>(0), ts);
        break;
#  endif

#  ifdef CONFIG_DAWN_DTYPE_CHAR
      case SObjectId::DTYPE_CHAR:
        printCharDataHelper(outstream_, io, ddim, &data->get<char>(0), ts);
        break;
#  endif

      default:
        fprintf(outstream_, "missing print support for dtype=%d\n", dtype);
        break;
    }

  fprintf(outstream_, "\n\n");

  return OK;
}
#endif

int CProtoShellPretty::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_SHELL_STD)
        {
          DAWNERR("Unsupported Shell config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_SHELL_CFG_IOBIND:
            {
              const size_t wpe = sizeof(SProtoShellIOBind) / 4;
              size_t j;

              for (j = 0; j < item->cfgid.s.size; j++)
                {
                  const SProtoShellIOBind *tmp =
                    reinterpret_cast<SProtoShellIOBind *>(item->data + j * wpe);

                  allocObject(tmp->objid);
                }

              break;
            }

          case CProtoShellPretty::PROTO_SHELL_CFG_PATH:
            {
              path = reinterpret_cast<const char *>(&item->data);
              break;
            }

          case CProtoShellPretty::PROTO_SHELL_CFG_PROMPT:
            {
              const char *tmp = reinterpret_cast<const char *>(&item->data);
              setPrompt(tmp);
              break;
            }

          default:
            {
              DAWNERR("Unsupported Shell config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
};

void CProtoShellPretty::allocObject(const SObjectId::UObjectId &obj)
{
  DAWNINFO("allocate object 0x%" PRIx32 "\n", obj.v);

  // Allocate object in map

  setObjectMapItem(obj.v, nullptr);
};

void CProtoShellPretty::setPrompt(const char *p)
{
  prompt = p;
}

void CProtoShellPretty::getAndPrintSeek(CIOCommon *io) const
{
  SObjectId::ObjectId objid = io->getIdV();
  size_t total = io->getDataSize();
  size_t offset = 0;
  io_ddata_t *chunk;
  int ret;

  fprintf(outstream_, "IO 0x%" PRIx32 " seekable data (%zu bytes):\n", objid, total);

  chunk = io->ddata_alloc(1, SHELL_SEEK_CHUNK_SIZE);
  if (chunk == nullptr)
    {
      fprintf(outstream_, "ddata_alloc failed\n");
      return;
    }

  while (offset < total)
    {
      const uint8_t *ptr;
      size_t avail;
      size_t chunk_bytes;
      size_t i;

      ret = io->getData(*chunk, 1, offset);
      if (ret < 0)
        {
          fprintf(outstream_, "getio seek failed at offset %zu: %d\n", offset, ret);
          break;
        }

      avail = total - offset;
      chunk_bytes = chunk->getDataSize() < avail ? chunk->getDataSize() : avail;
      ptr = static_cast<const uint8_t *>(chunk->getDataPtr());

      for (i = 0; i < chunk_bytes; i += SHELL_SEEK_LINE_BYTES)
        {
          size_t line_len =
            (chunk_bytes - i) < SHELL_SEEK_LINE_BYTES ? (chunk_bytes - i) : SHELL_SEEK_LINE_BYTES;
          size_t j;

          fprintf(outstream_, "%08zx: ", offset + i);

          for (j = 0; j < SHELL_SEEK_LINE_BYTES; j++)
            {
              if (j < line_len)
                {
                  fprintf(outstream_, "%02x ", ptr[i + j]);
                }
              else
                {
                  fprintf(outstream_, "   ");
                }

              if (j == 7)
                {
                  fprintf(outstream_, " ");
                }
            }

          fprintf(outstream_, " ");

          for (j = 0; j < line_len; j++)
            {
              uint8_t c = ptr[i + j];
              fprintf(outstream_, "%c", (c >= 0x20 && c < 0x7f) ? (char)c : '.');
            }

          fprintf(outstream_, "\n");
        }

      offset += chunk_bytes;
    }

  free(chunk);
}

void CProtoShellPretty::getAndPrint(CIOCommon *io) const
{
  SObjectId::ObjectId objid = io->getIdV();
  size_t ddim = io->getDataDim();
  int dtype = io->getDtype();
  (void)ddim;

  // Check if we can read

  if (io->isRead() == false)
    {
      fprintf(outstream_, "IO 0x%" PRIx32 "is write-only\n", objid);
      return;
    }

  // Seekable IOs use hex dump output

  if (io->isSeekable())
    {
      getAndPrintSeek(io);
      return;
    }

  fprintf(outstream_, "IO 0x%" PRIx32 " data:\n\t", objid);

  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        getAndPrintTyped<bool>(outstream_, io, ddim, "%" PRId8 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        getAndPrintTyped<int8_t>(outstream_, io, ddim, "%" PRId8 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        getAndPrintTyped<uint8_t>(outstream_, io, ddim, "%" PRId8 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        getAndPrintTyped<int16_t>(outstream_, io, ddim, "%" PRId16 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        getAndPrintTyped<uint16_t>(outstream_, io, ddim, "%" PRId16 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        getAndPrintTyped<int32_t>(outstream_, io, ddim, "%" PRId32 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        getAndPrintTyped<uint32_t>(outstream_, io, ddim, "%" PRId32 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        getAndPrintTyped<uint64_t>(outstream_, io, ddim, "%" PRId64 " ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        getAndPrintTyped<float>(outstream_, io, ddim, "%.4f ");
        break;
#endif
#ifdef CONFIG_DAWN_DTYPE_CHAR
      case SObjectId::DTYPE_CHAR:
        getAndPrintChar(outstream_, io, ddim);
        break;
#endif
      default:
        fprintf(outstream_, "missing print support for dtype=%d\n", dtype);
        break;
    }

  fprintf(outstream_, "\n");
}

void CProtoShellPretty::getcfgAndPrint(CObject *obj) const
{
  const CDescObject &desc = obj->getDesc();

  for (size_t i = 0; i < desc.getSizeBytes() / 4; i++)
    {
      fprintf(outstream_, "  0x%08" PRIx32 "\n", desc.getAtOffset(i));
    }
}

void CProtoShellPretty::cmdHelp()
{
  fprintf(outstream_, "Proto SHELL help:\n");
  fprintf(outstream_, "\t help - print this message\n");
  fprintf(outstream_, "\t exit - exit\n");
  fprintf(outstream_, "\t info - show shell-bound IO inventory\n");
  fprintf(outstream_, "\t getio <IO objectID> - get value for a given objectID\n");
  fprintf(outstream_, "\t setio <IO objectID> <v1..vN> - set IO value words\n");
  fprintf(outstream_, "\t getioloop - get all objects in loop\n");
#ifdef CONFIG_DAWN_IO_NOTIFY
  fprintf(outstream_, "\t getionotify <objectID> - get io with notification\n");
#endif
  fprintf(outstream_, "\t getcfg <objectID> [cfgID] - get object configuration\n");
  fprintf(outstream_, "\t setcfg <objectID> <cfgID> <v1..vN> - set object config words\n");
#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
  fprintf(outstream_, "\n Object Inspector:\n");
  fprintf(outstream_, "\t list [io|prog|proto] [verbose] - show object inventory\n");
  fprintf(outstream_, "\t inspect <objectID> - detailed object introspection\n");
  fprintf(outstream_, "\t tree - show object hierarchy\n");
  fprintf(outstream_, "\t stats - show runtime statistics\n");
#endif
  fprintf(outstream_, "\n\t NOTE: all values must be in hex format: 0xYYYYYYYY\n");
}

void CProtoShellPretty::cmdExit()
{
  CShutdown::request();
  workerThread().requestStop();
}

void CProtoShellPretty::cmdInfo()
{
  fprintf(outstream_, "INFO: shell IO bindings\n");
  fprintf(outstream_, "  Name           ObjID      DType[Dim] Flags\n");
  fprintf(outstream_, "  -------------- ---------- ---------- ------\n");

  for (auto const &[id, io] : getIOMap())
    {
      if (io == nullptr)
        {
          fprintf(
            outstream_, "  %-14s 0x%08" PRIx32 " %-10s %s\n", "(unbound)", id, "-", "unbound");
          continue;
        }

      fprintf(outstream_,
              "  %-14s 0x%08" PRIx32 " %s[%zu] %c%c%c%c%c\n",
#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
              io->getName(),
#else
              "-",
#endif
              io->getIdV(),
              SObjectId::dtypeToString(io->getDtype()),
              io->getDataDim(),
              io->isRead() ? 'R' : '-',
              io->isWrite() ? 'W' : '-',
              io->isNotify() ? 'N' : '-',
              io->isTimestamp() ? 'T' : '-',
              io->getCfgFlag() ? 'C' : '-');
    }

  fprintf(outstream_, "Flags: R=Read W=Write N=Notify T=Timestamp C=Config\n");
#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
  fprintf(outstream_, "Use 'list io' for all runtime IOs or 'list verbose' for details.\n");
#endif
}

void CProtoShellPretty::cmdSetio(const char *arg)
{
  SObjectId::ObjectId objid = 0;
  CIOCommon *io = nullptr;
  io_ddata_t *data = nullptr;
  char argbuf[DAWN_PROTO_SHELL_LINELEN];
  char *saveptr = nullptr;
  char *token;
  size_t words;
  size_t data_size;
  size_t elem_size;
  size_t i;
  int ret;

  if (arg == nullptr || *arg == '\0')
    {
      fprintf(outstream_, "invalid argument\n");
      return;
    }

  strncpy(argbuf, arg, sizeof(argbuf) - 1);
  argbuf[sizeof(argbuf) - 1] = '\0';

  token = strtok_r(argbuf, " \t", &saveptr);
  if (!parseU32Token(token, &objid))
    {
      fprintf(outstream_, "invalid argument: %s\n", arg);
      return;
    }

  token = strtok_r(nullptr, " \t", &saveptr);
  if (token == nullptr)
    {
      fprintf(outstream_,
              "invalid argument: %s."
              " Must be string in 0xXXXXXXXX format\n",
              arg);
      return;
    }

  // Get IO

  io = getIO(objid);
  if (io == nullptr)
    {
      fprintf(outstream_, "no IO with objID = 0x%" PRIx32 "\n", objid);
      return;
    }

  if (io->getIdV() != objid)
    {
      fprintf(outstream_,
              "IO ID mismatch: expected 0x%" PRIx32 ", got 0x%" PRIx32 "\n",
              objid,
              io->getIdV());
      return;
    }

  // Check if we can write

  if (io->isWrite() == false)
    {
      fprintf(outstream_, "IO 0x%" PRIx32 "is read-only\n", objid);
      return;
    }

  words = io->getDataDim();
  if (words == 0 || words > GETANDPRINT_DIM_MAX)
    {
      fprintf(outstream_, "unsupported io dim %zu\n", words);
      return;
    }

  data_size = io->getDataSize();
  if (data_size == 0 || data_size % words != 0)
    {
      fprintf(outstream_, "unsupported io data shape size=%zu dim=%zu\n", data_size, words);
      return;
    }

  elem_size = data_size / words;
  if (elem_size == 0 || elem_size > 4)
    {
      fprintf(outstream_, "io set supported only for element size <= 4\n");
      return;
    }

  data = io->ddata_alloc(1);
  if (data == nullptr)
    {
      fprintf(outstream_, "setio buffer alloc failed\n");
      return;
    }

  for (i = 0; i < words; i++)
    {
      uint32_t value;

      if (i > 0)
        {
          token = strtok_r(nullptr, " \t", &saveptr);
        }

      if (!parseU32Token(token, &value))
        {
          fprintf(
            outstream_, "invalid data: expected %zu words for IO 0x%" PRIx32 "\n", words, objid);
          delete data;
          return;
        }

      std::memcpy(static_cast<uint8_t *>(data->getDataPtr()) + i * elem_size, &value, elem_size);
    }

  if (strtok_r(nullptr, " \t", &saveptr) != nullptr)
    {
      fprintf(outstream_, "too many data words for IO 0x%" PRIx32 "\n", objid);
      delete data;
      return;
    }

  ret = io->setData(*data);
  if (ret < 0)
    {
      fprintf(outstream_, "setio failed %d\n", ret);
    }

  delete data;
}

void CProtoShellPretty::cmdGetio(const char *arg)
{
  SObjectId::ObjectId objid = 0;
  CIOCommon *io;

  // Get objectid

  if (!parseU32Token(arg, &objid))
    {
      fprintf(outstream_, "invalid argument: %s\n", arg);
      return;
    }

  // Get IO

  io = getIO(objid);
  if (io == nullptr)
    {
      fprintf(outstream_, "no IO with objID = 0x%" PRIx32 "\n", objid);
      return;
    }

  if (io->getIdV() != objid)
    {
      fprintf(outstream_,
              "IO ID mismatch: expected 0x%" PRIx32 ", got 0x%" PRIx32 "\n",
              objid,
              io->getIdV());
      return;
    }

  // Get data and print on stdout

  getAndPrint(io);
}

void CProtoShellPretty::cmdGetioloop(const char *arg)
{
  int times = 10;

  do
    {
      fprintf(outstream_, "\nProto 0x%" PRIx32 "\n", getIdV());

      for (auto const &[id, io] : getIOMap())
        {
          if (io == nullptr)
            {
              fprintf(outstream_, "IO 0x%" PRIx32 " is not bound\n", id);
              continue;
            }

          getAndPrint(io);
        }

      times--;
      std::this_thread::sleep_for(1000ms);
    }
  while (times > 0);
}

#ifdef CONFIG_DAWN_IO_NOTIFY
void CProtoShellPretty::cmdGetioNotify(const char *arg)
{
  SObjectId::ObjectId objid = 0;
  CIOCommon *io;
  int ret;

  // Get objectid

  if (!parseU32Token(arg, &objid))
    {
      fprintf(outstream_, "invalid argument: %s\n", arg);
      return;
    }

  // Get IO

  io = getIO(objid);
  if (io == nullptr)
    {
      fprintf(outstream_, "no IO with objID = 0x%" PRIx32 "\n", objid);
      return;
    }

  if (io->getIdV() != objid)
    {
      fprintf(outstream_,
              "IO ID mismatch: expected 0x%" PRIx32 ", got 0x%" PRIx32 "\n",
              objid,
              io->getIdV());
      return;
    }

  // Set notifier

  ret = io->setNotifier(notifierCb, 0, io);
  if (ret < 0)
    {
      fprintf(outstream_, "ERROR: set notifier failed for objId = 0x%" PRIx32 "\n", objid);
      return;
    }
}
#endif

void CProtoShellPretty::cmdSetcfg(const char *arg)
{
  SObjectId::ObjectId objid = 0;
  SObjectCfg::ObjectCfgId objcfg = 0;
  CObject *obj;
  std::vector<uint32_t> data;
  char argbuf[DAWN_PROTO_SHELL_LINELEN];
  char *saveptr = nullptr;
  char *token;
  size_t words;
  size_t i;
  int ret;

  if (arg == nullptr || *arg == '\0')
    {
      fprintf(outstream_, "invalid arguments\n");
      return;
    }

  strncpy(argbuf, arg, sizeof(argbuf) - 1);
  argbuf[sizeof(argbuf) - 1] = '\0';

  token = strtok_r(argbuf, " \t", &saveptr);
  if (!parseU32Token(token, &objid))
    {
      fprintf(outstream_, "invalid object ID: %s\n", arg);
      return;
    }

  token = strtok_r(nullptr, " \t", &saveptr);
  if (!parseU32Token(token, &objcfg))
    {
      fprintf(outstream_, "invalid cfg ID: %s\n", arg);
      return;
    }

  words = SObjectCfg::objectCfgGetSize(objcfg);
  if (words == 0)
    {
      fprintf(outstream_, "invalid cfg word count for cfgID = 0x%" PRIx32 "\n", objcfg);
      return;
    }

  data.resize(words);
  for (i = 0; i < words; i++)
    {
      token = strtok_r(nullptr, " \t", &saveptr);
      if (!parseU32Token(token, &data[i]))
        {
          fprintf(outstream_,
                  "invalid data: expected %zu words for cfgID 0x%" PRIx32 "\n",
                  words,
                  objcfg);
          return;
        }
    }

  if (strtok_r(nullptr, " \t", &saveptr) != nullptr)
    {
      fprintf(outstream_, "too many data words for cfgID 0x%" PRIx32 "\n", objcfg);
      return;
    }

  obj = getObject(objid);
  if (obj == nullptr)
    {
      fprintf(outstream_, "no object with objID = 0x%" PRIx32 "\n", objid);
      return;
    }

  ret = obj->setObjConfig(objcfg, data.data(), words);
  if (ret < 0)
    {
      fprintf(outstream_, "failed to update objectCfg = 0x%" PRIx32 "\n", objcfg);
      return;
    }
}

void CProtoShellPretty::cmdGetcfg(const char *arg)
{
  SObjectId::ObjectId objid = 0;
  SObjectCfg::ObjectCfgId objcfg = 0;
  CObject *obj;
  std::vector<uint32_t> data;
  char argbuf[DAWN_PROTO_SHELL_LINELEN];
  char *saveptr = nullptr;
  char *token;
  size_t words;
  size_t i;

  if (arg == nullptr || *arg == '\0')
    {
      fprintf(outstream_, "invalid arguments\n");
      return;
    }

  strncpy(argbuf, arg, sizeof(argbuf) - 1);
  argbuf[sizeof(argbuf) - 1] = '\0';

  token = strtok_r(argbuf, " \t", &saveptr);
  if (!parseU32Token(token, &objid))
    {
      fprintf(outstream_, "invalid object ID: %s\n", arg);
      return;
    }

  obj = getObject(objid);
  if (obj == nullptr)
    {
      fprintf(outstream_, "no object with objID = 0x%" PRIx32 "\n", objid);
      return;
    }

  token = strtok_r(nullptr, " \t", &saveptr);
  if (token == nullptr)
    {
      getcfgAndPrint(obj);
      return;
    }

  if (!parseU32Token(token, &objcfg))
    {
      fprintf(outstream_, "invalid cfg ID: %s\n", arg);
      return;
    }

  if (strtok_r(nullptr, " \t", &saveptr) != nullptr)
    {
      fprintf(outstream_, "invalid argument: %s\n", arg);
      return;
    }

  words = SObjectCfg::objectCfgGetSize(objcfg);
  if (words == 0)
    {
      fprintf(outstream_, "invalid cfg word count for cfgID = 0x%" PRIx32 "\n", objcfg);
      return;
    }

  data.resize(words);
  if (obj->getObjConfig(objcfg, data.data(), words) < 0)
    {
      fprintf(outstream_, "failed to read objectCfg = 0x%" PRIx32 "\n", objcfg);
      return;
    }

  fprintf(outstream_, "CFG 0x%" PRIx32 ":\n", objcfg);
  for (i = 0; i < words; i++)
    {
      fprintf(outstream_, "\t0x%08" PRIx32 "\n", data[i]);
    }
}

void CProtoShellPretty::cmdHandle(char *cmd, const char *arg)
{
  std::string scmd = cmd;

  // fprintf(outstream_, "cmd = %s arg = %s\n", cmd, arg);

  switch (cmdmap[scmd])
    {
      case PROTO_SHELL_CMDID_HELP:
        {
          cmdHelp();
          break;
        }

      case PROTO_SHELL_CMDID_EXIT:
        {
          cmdExit();
          break;
        }

      case PROTO_SHELL_CMDID_INFO:
        {
          cmdInfo();
          break;
        }

      case PROTO_SHELL_CMDID_SETIO:
        {
          cmdSetio(arg);
          break;
        }

      case PROTO_SHELL_CMDID_GETIO:
        {
          cmdGetio(arg);
          break;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY
      case PROTO_SHELL_CMDID_GETIONOTIFY:
        {
          cmdGetioNotify(arg);
          break;
        }
#endif

      case PROTO_SHELL_CMDID_GETIOLOOP:
        {
          cmdGetioloop(arg);
          break;
        }

      case PROTO_SHELL_CMDID_SETCFG:
        {
          cmdSetcfg(arg);
          break;
        }

      case PROTO_SHELL_CMDID_GETCFG:
        {
          cmdGetcfg(arg);
          break;
        }

#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT
      case PROTO_SHELL_CMDID_LIST:
        {
          cmdList(arg);
          break;
        }

      case PROTO_SHELL_CMDID_INSPECT:
        {
          cmdInspect(arg);
          break;
        }

      case PROTO_SHELL_CMDID_TREE:
        {
          cmdTree(arg);
          break;
        }

      case PROTO_SHELL_CMDID_STATS:
        {
          cmdStats(arg);
          break;
        }
#endif // CONFIG_DAWN_PROTO_SHELL_INSPECT

      default:
        {
          fprintf(outstream_, "ERROR:invalid command %s\n", cmd);
          cmdHelp();
          break;
        }
    }
}

void CProtoShellPretty::thread()
{
  char buffer[DAWN_PROTO_SHELL_LINELEN];
  char *cmd;
  char *arg;

  // Initialize thread

  fprintf(outstream_, "\n*Start DAWN Shell*\n\n");

  // Loop until stop called

  do
    {
      if (prompt)
        {
          fprintf(outstream, "%s", prompt);
          fflush(stdout);
        }

      auto len = readline_stream(buffer, sizeof(buffer), instream, outstream);
      if (len > 0)
        {
          // Handle simple ENTER

          if (len == 1)
            {
              continue;
            }

          // Remove trailing line endings (\n, \r, or CRLF)

          buffer[len] = '\0';
          while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
            {
              buffer[len - 1] = '\0';
              len -= 1;
            }

          if (len == 0)
            {
              continue;
            }

          // Parse the command from the argument

          cmd = strtok_r(buffer, " \r\n", &arg);
          if (cmd == nullptr)
            {
              continue;
            }

          // Remove leading spaces from arg

          while (*arg == ' ')
            {
              arg++;
            }

          // Handle command

          cmdHandle(cmd, arg);
        }
    }
  while (!workerThread().shouldQuit());

  // Mark thread quit done

  workerThread().markThreadFinished();
}

CProtoShellPretty::~CProtoShellPretty()
{
  // Make sure that thread is stopped

  stopWorkerThread();

  // Close streams

  fclose(instream);
  fclose(outstream);

  if (CProtoShellPretty::inst > 0)
    {
      CProtoShellPretty::inst -= 1;
    }
}

int CProtoShellPretty::configure()
{
  int ret;

  if (CProtoShellPretty::inst != 0)
    {
      DAWNERR("Only one shell instance is supported\n");
      return -EBUSY;
    }

  CProtoShellPretty::inst += 1;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Shell configure failed (error %d)\n", ret);
      CProtoShellPretty::inst -= 1;
      return ret;
    }

  if (path)
    {
      DAWNINFO("CProtoShellPretty path= %s\n", path);

      instream = fopen(path, "r");
      if (instream == nullptr)
        {
          DAWNERR("Failed to open input stream: %s\n", path);
          CProtoShellPretty::inst -= 1;
          return -EIO;
        }

      outstream = fopen(path, "w");
      if (outstream == nullptr)
        {
          DAWNERR("Failed to open output stream: %s\n", path);
          fclose(instream);
          instream = nullptr;
          CProtoShellPretty::inst -= 1;
          return -EIO;
        }
    }
  else
    {
      // Default streams

      instream = stdin;
      outstream = stdout;
    }

  // Store reference to output stream

  outstream_ = outstream;

  return OK;
}

int CProtoShellPretty::doStart()
{
  return startWorkerThread([this]() { thread(); });
};

int CProtoShellPretty::doStop()
{
  return stopWorkerThread();
}

bool CProtoShellPretty::hasThread() const
{
  return workerThreadRunning();
}
