// dawn/src/proto/shell/inspect.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/shell/pretty.hxx"

#ifdef CONFIG_DAWN_PROTO_SHELL_INSPECT

#  include <cstdlib>
#  include <cstring>

#  include "dawn/common/object.hxx"
#  include "dawn/common/objectid.hxx"
#  include "dawn/dev/inspector.hxx"
#  include "dawn/io/common.hxx"
#  include "dawn/io/config.hxx"

using namespace dawn;

#  define DAWN_PROTO_SHELL_LIST_ARG_MAX 64

void CProtoShellPretty::cmdList(const char *arg)
{
  CDevInspector *inspector;
  size_t count;
  size_t i;
  const CIOCommon *io;
  const CObject *obj;
  bool verbose;
  bool show_io;
  bool show_prog;
  bool show_proto;

  verbose = false;
  show_io = true;
  show_prog = true;
  show_proto = true;

  if (arg != nullptr && arg[0] != '\0')
    {
      char argbuf[DAWN_PROTO_SHELL_LIST_ARG_MAX];
      char *saveptr = nullptr;
      char *token;
      bool filtered = false;

      std::strncpy(argbuf, arg, sizeof(argbuf) - 1);
      argbuf[sizeof(argbuf) - 1] = '\0';

      token = strtok_r(argbuf, " \t", &saveptr);
      while (token != nullptr)
        {
          if (std::strcmp(token, "verbose") == 0 || std::strcmp(token, "full") == 0)
            {
              verbose = true;
            }
          else if (std::strcmp(token, "io") == 0)
            {
              if (!filtered)
                {
                  show_io = false;
                  show_prog = false;
                  show_proto = false;
                  filtered = true;
                }
              show_io = true;
            }
          else if (std::strcmp(token, "prog") == 0)
            {
              if (!filtered)
                {
                  show_io = false;
                  show_prog = false;
                  show_proto = false;
                  filtered = true;
                }
              show_prog = true;
            }
          else if (std::strcmp(token, "proto") == 0)
            {
              if (!filtered)
                {
                  show_io = false;
                  show_prog = false;
                  show_proto = false;
                  filtered = true;
                }
              show_proto = true;
            }
          else
            {
              fprintf(outstream, "Usage: list [io|prog|proto] [verbose]\n");
              return;
            }

          token = strtok_r(nullptr, " \t", &saveptr);
        }
    }

  inspector = CDevInspector::getInst();

  fprintf(outstream, "\n");
  fprintf(outstream, "Object Inventory\n");
  fprintf(outstream, "================\n\n");

  // List IO objects

  if (show_io)
    {
      count = inspector->getIOCount();
      fprintf(outstream, "IOs: %zu objects\n", count);

      if (count > 0)
        {
#  ifdef CONFIG_DAWN_IO_HAS_STATS
          if (verbose)
            {
              fprintf(outstream,
                      "  Name           ObjID      Type Cls  DType  Dim Size Flags  "
                      "Stats (R/W/E)\n");
              fprintf(outstream,
                      "  -------------- ---------- ---- ---- ------ --- ---- ------ "
                      "--------------\n");
            }
          else
            {
              fprintf(outstream, "  Name           ObjID      DType[Dim]  Cls Flags  R/W/E\n");
              fprintf(outstream, "  -------------- ---------- ----------- --- ------ -------\n");
            }
#  else
          if (verbose)
            {
              fprintf(outstream, "  Name           ObjID      Type Cls  DType  Dim Size Flags\n");
              fprintf(outstream, "  -------------- ---------- ---- ---- ------ --- ---- ------\n");
            }
          else
            {
              fprintf(outstream, "  Name           ObjID      DType[Dim]  Cls Flags\n");
              fprintf(outstream, "  -------------- ---------- ----------- --- ------\n");
            }
#  endif

          for (i = 0; i < count; i++)
            {
              io = inspector->getIO(i);
              if (io == nullptr)
                {
                  continue;
                }

              if (verbose)
                {
                  fprintf(outstream, "  %-14s ", io->getName());
                  fprintf(outstream, "0x%08" PRIx32 " ", io->getIdV());
                  fprintf(outstream,
                          "%4d %4d %-6s ",
                          io->getType(),
                          io->getCls(),
                          SObjectId::dtypeToString(io->getDtype()));
                  fprintf(outstream, "%3zu %4zu ", io->getDataDim(), io->getDataSize());
                }
              else
                {
                  fprintf(outstream,
                          "  %-14s 0x%08" PRIx32 " %s[%zu] %3d ",
                          io->getName(),
                          io->getIdV(),
                          SObjectId::dtypeToString(io->getDtype()),
                          io->getDataDim(),
                          io->getCls());
                }

              fprintf(outstream,
                      "%c%c%c%c%c",
                      io->isRead() ? 'R' : '-',
                      io->isWrite() ? 'W' : '-',
                      io->isNotify() ? 'N' : '-',
                      io->isTimestamp() ? 'T' : '-',
                      io->getCfgFlag() ? 'C' : '-');

#  ifdef CONFIG_DAWN_IO_HAS_STATS
              const auto &stats = io->getStats();
              if (verbose)
                {
                  fprintf(outstream,
                          "  %6" PRIu32 "/%6" PRIu32 "/%6" PRIu32 "\n",
                          stats.read_count,
                          stats.write_count,
                          stats.error_count);
                }
              else
                {
                  fprintf(outstream,
                          "  %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
                          stats.read_count,
                          stats.write_count,
                          stats.error_count);
                }
#  else
              fprintf(outstream, "\n");
#  endif
            }
        }

      fprintf(outstream, "\n");
    }

  // List PROG objects

  if (show_prog)
    {
      count = inspector->getProgCount();
      fprintf(outstream, "PROGs: %zu objects\n", count);

      if (count > 0)
        {
          if (verbose)
            {
              fprintf(outstream, "  Name           ObjID      Type Cls  Running\n");
              fprintf(outstream, "  -------------- ---------- ---- ---- -------\n");
            }
          else
            {
              fprintf(outstream, "  Name           ObjID      Cls State\n");
              fprintf(outstream, "  -------------- ---------- --- -------\n");
            }

          for (i = 0; i < count; i++)
            {
              obj = inspector->getProg(i);
              if (obj == nullptr)
                {
                  continue;
                }

              if (verbose)
                {
                  fprintf(outstream, "  %-14s ", obj->getName());
                  fprintf(outstream, "0x%08" PRIx32 " ", obj->getIdV());
                  fprintf(outstream,
                          "%4d %4d %7s\n",
                          obj->getType(),
                          obj->getCls(),
                          obj->getState() == CObject::STATE_RUNNING ? "YES" : "NO");
                }
              else
                {
                  fprintf(outstream,
                          "  %-14s 0x%08" PRIx32 " %3d %s\n",
                          obj->getName(),
                          obj->getIdV(),
                          obj->getCls(),
                          obj->getState() == CObject::STATE_RUNNING ? "RUNNING" : "STOPPED");
                }
            }
        }

      fprintf(outstream, "\n");
    }

  // List PROTO objects

  if (show_proto)
    {
      count = inspector->getProtoCount();
      fprintf(outstream, "PROTOs: %zu objects\n", count);

      if (count > 0)
        {
          if (verbose)
            {
              fprintf(outstream, "  Name           ObjID      Type Cls  Running\n");
              fprintf(outstream, "  -------------- ---------- ---- ---- -------\n");
            }
          else
            {
              fprintf(outstream, "  Name           ObjID      Cls State\n");
              fprintf(outstream, "  -------------- ---------- --- -------\n");
            }

          for (i = 0; i < count; i++)
            {
              obj = inspector->getProto(i);
              if (obj == nullptr)
                {
                  continue;
                }

              if (verbose)
                {
                  fprintf(outstream, "  %-14s ", obj->getName());
                  fprintf(outstream, "0x%08" PRIx32 " ", obj->getIdV());
                  fprintf(outstream,
                          "%4d %4d %7s\n",
                          obj->getType(),
                          obj->getCls(),
                          obj->getState() == CObject::STATE_RUNNING ? "YES" : "NO");
                }
              else
                {
                  fprintf(outstream,
                          "  %-14s 0x%08" PRIx32 " %3d %s\n",
                          obj->getName(),
                          obj->getIdV(),
                          obj->getCls(),
                          obj->getState() == CObject::STATE_RUNNING ? "RUNNING" : "STOPPED");
                }
            }
        }

      fprintf(outstream, "\n");
    }

  fprintf(outstream, "Flags: R=Read W=Write N=Notify T=Timestamp C=Config\n");
#  ifdef CONFIG_DAWN_IO_HAS_STATS
  fprintf(outstream, "Stats: R=Reads W=Writes E=Errors\n");
#  endif
  fprintf(outstream, "Use 'list io|prog|proto' to filter and 'list verbose' for details.\n");
  fprintf(outstream, "\n");
}

void CProtoShellPretty::cmdInspect(const char *arg)
{
  char *endptr;
  uint32_t objid_val;
  CDevInspector *inspector;
  const CObject *obj;
  const char *handler_type;

  if (!arg || arg[0] == '\0')
    {
      fprintf(outstream, "Usage: inspect <object_id>\n");
      fprintf(outstream, "  object_id: hex (0xNNNNNNNN) or decimal\n");
      return;
    }

  // Parse object ID

  objid_val = strtoul(arg, &endptr, 0);
  if (*endptr != '\0' && *endptr != ' ' && *endptr != '\t')
    {
      fprintf(outstream, "Error: invalid object ID '%s'\n", arg);
      return;
    }

  // Find object

  inspector = CDevInspector::getInst();
  obj = inspector->findObject(objid_val);

  if (!obj)
    {
      fprintf(outstream, "Error: object 0x%08" PRIx32 " not found\n", objid_val);
      return;
    }

  // Determine handler type

  switch (obj->getType())
    {
      case SObjectId::OBJTYPE_IO:
        handler_type = "IO";
        break;
      case SObjectId::OBJTYPE_PROG:
        handler_type = "PROG";
        break;
      case SObjectId::OBJTYPE_PROTO:
        handler_type = "PROTO";
        break;
      default:
        handler_type = "UNKNOWN";
        break;
    }

  // Display detailed information

  fprintf(outstream, "\n");
  fprintf(outstream, "Object Details\n");
  fprintf(outstream, "==============\n\n");
  fprintf(outstream, "  Name:         %s\n", obj->getName());
  fprintf(outstream, "  Object ID:    0x%08" PRIx32 "\n", obj->getIdV());
  fprintf(outstream, "  Handler:      %s\n", handler_type);
  fprintf(outstream, "\n");

  // Object ID breakdown

  fprintf(outstream, "  Type:         0x%02x ", obj->getType());
  switch (obj->getType())
    {
      case SObjectId::OBJTYPE_IO:
        fprintf(outstream, "(IO)\n");
        break;
      case SObjectId::OBJTYPE_PROG:
        fprintf(outstream, "(PROG)\n");
        break;
      case SObjectId::OBJTYPE_PROTO:
        fprintf(outstream, "(PROTO)\n");
        break;
      default:
        fprintf(outstream, "(UNKNOWN)\n");
        break;
    }

  fprintf(outstream, "  Class:        0x%03x (%u)\n", obj->getCls(), obj->getCls());
  fprintf(outstream, "  Data Type:    0x%02x ", obj->getDtype());

  // Display data type name

  if (obj->getDtype() == SObjectId::DTYPE_ANY)
    {
      fprintf(outstream, "(%s)\n", SObjectId::dtypeToString(obj->getDtype()));
    }
  else if (obj->getDtype() == SObjectId::DTYPE_BLOCK)
    {
      fprintf(outstream, "(%s)\n", SObjectId::dtypeToString(obj->getDtype()));
    }
  else
    {
      fprintf(outstream,
              "(%s, %zu bytes)\n",
              SObjectId::dtypeToString(obj->getDtype()),
              obj->getDtypeSize());
    }

  if (obj->getType() == SObjectId::OBJTYPE_IO)
    {
      const CIOCommon *io = static_cast<const CIOCommon *>(obj);
      fprintf(outstream,
              "  Dimension:    %zu (item size %zu bytes)\n",
              io->getDataDim(),
              io->getDataSize());
    }

  fprintf(outstream, "  Flags:        0x%02x\n", obj->getFlags());
  fprintf(outstream, "  Instance:     %u\n", obj->getPriv());
  fprintf(outstream, "\n");

  // Configuration info

  if (obj->getCfgFlag())
    {
      fprintf(outstream, "  Config:       Yes\n");
    }
  else
    {
      fprintf(outstream, "  Config:       No\n");
    }

  fprintf(outstream, "\n");

  // ConfigIO binding information (only for ConfigIO objects)

  if (obj->getType() == SObjectId::OBJTYPE_IO && obj->getCls() == CIOCommon::IO_CLASS_CONFIG)
    {
      const CIOConfig *cfg_io = static_cast<const CIOConfig *>(obj);

      fprintf(outstream, "ConfigIO Bindings\n");
      fprintf(outstream, "-----------------\n");

      if (cfg_io->map.empty())
        {
          fprintf(outstream, "  (no bindings)\n");
        }
      else
        {
          for (const auto &[id, bound_obj] : cfg_io->map)
            {
              if (bound_obj)
                {
                  const char *type_str = "UNKNOWN";

                  switch (bound_obj->getType())
                    {
                      case SObjectId::OBJTYPE_IO:
                        type_str = "IO";
                        break;
                      case SObjectId::OBJTYPE_PROG:
                        type_str = "PROG";
                        break;
                      case SObjectId::OBJTYPE_PROTO:
                        type_str = "PROTO";
                        break;
                    }

                  fprintf(outstream,
                          "  -> %s: %s (0x%08" PRIx32 ")\n",
                          type_str,
                          bound_obj->getName(),
                          bound_obj->getIdV());
                }
              else
                {
                  fprintf(outstream, "  -> 0x%08" PRIx32 " (not bound yet)\n", id);
                }
            }
        }

      fprintf(outstream, "\n");
    }

    // Runtime statistics (only for IO objects)

#  ifdef CONFIG_DAWN_IO_HAS_STATS
  if (obj->getType() == SObjectId::OBJTYPE_IO)
    {
      const CIOCommon *io = static_cast<const CIOCommon *>(obj);
      const CIOCommon::IOStats &stats = io->getStats();
      fprintf(outstream, "Statistics\n");
      fprintf(outstream, "----------\n");
      fprintf(outstream, "  Reads:        %" PRIu32 "\n", stats.read_count);
      fprintf(outstream, "  Writes:       %" PRIu32 "\n", stats.write_count);
      fprintf(outstream, "  Errors:       %" PRIu32 "\n", stats.error_count);
      fprintf(outstream, "\n");
    }
#  endif
}

void CProtoShellPretty::cmdTree(const char *arg)
{
  CDevInspector *inspector;
  size_t proto_count;
  size_t prog_count;
  size_t io_count;
  size_t i;
  size_t j;
  size_t k;
  const CObject *proto;
  const CObject *prog;
  const CIOCommon *io;
  const CObject *bound_objs[128];
  size_t binding_count;
  const CIOCommon *io_bindings[128];
  size_t io_binding_count;
  bool is_bound;
  bool has_unbound;
  const CObject *bound_obj;

  (void)arg;

  inspector = CDevInspector::getInst();

  fprintf(outstream, "\n");
  fprintf(outstream, "Data Flow Tree\n");
  fprintf(outstream, "==============\n\n");

  // Show PROTO objects at top level (communication endpoints)

  proto_count = inspector->getProtoCount();

  if (proto_count == 0)
    {
      fprintf(outstream, "(no protocol objects)\n\n");
    }
  else
    {
      for (i = 0; i < proto_count; i++)
        {
          proto = inspector->getProto(i);
          if (proto == nullptr)
            {
              continue;
            }

          fprintf(outstream, "PROTO: %s\n", proto->getName());

          binding_count = inspector->getProtoBindings(i, bound_objs, 128);

          if (binding_count > 0)
            {
              for (j = 0; j < binding_count; j++)
                {
                  const bool is_last = (j == binding_count - 1);
                  const char *branch = is_last ? "└──" : "├──";
                  const char *cont = is_last ? "    " : "│   ";

                  bound_obj = bound_objs[j];

                  // Check if this is a PROG or IO object

                  if (bound_obj->getType() == SObjectId::OBJTYPE_PROG)
                    {
                      fprintf(outstream, "%s PROG: %s\n", branch, bound_obj->getName());

                      // Show PROG's bound IO objects - find prog index

                      for (k = 0; k < inspector->getProgCount(); k++)
                        {
                          if (inspector->getProg(k)->getIdV() == bound_obj->getIdV())
                            {
                              io_binding_count = inspector->getProgIOBindings(k, io_bindings, 128);

                              if (io_binding_count > 0)
                                {
                                  size_t io_idx;

                                  for (io_idx = 0; io_idx < io_binding_count; io_idx++)
                                    {
                                      const bool io_last = (io_idx == io_binding_count - 1);
                                      const char *io_branch = io_last ? "└──" : "├──";

                                      fprintf(outstream,
                                              "%s    %s IO: %s\n",
                                              cont,
                                              io_branch,
                                              io_bindings[io_idx]->getName());
                                    }
                                }

                              break;
                            }
                        }
                    }
                  else if (bound_obj->getType() == SObjectId::OBJTYPE_IO)
                    {
                      fprintf(outstream, "%s IO: %s\n", branch, bound_obj->getName());
                    }
                }
            }
          else
            {
              fprintf(outstream, "    (no bindings)\n");
            }

          fprintf(outstream, "\n");
        }
    }

  // Show unbound PROG objects

  prog_count = inspector->getProgCount();
  has_unbound = false;

  for (i = 0; i < prog_count; i++)
    {
      prog = inspector->getProg(i);
      if (prog == nullptr)
        {
          continue;
        }

      is_bound = false;

      // Check if this PROG is bound to any PROTO

      for (j = 0; j < proto_count; j++)
        {
          binding_count = inspector->getProtoBindings(j, bound_objs, 128);

          for (k = 0; k < binding_count; k++)
            {
              if (bound_objs[k]->getIdV() == prog->getIdV())
                {
                  is_bound = true;
                  break;
                }
            }

          if (is_bound)
            {
              break;
            }
        }

      if (!is_bound)
        {
          if (!has_unbound)
            {
              fprintf(outstream, "Unbound PROG objects:\n");
              has_unbound = true;
            }

          fprintf(outstream, "  * %s\n", prog->getName());

          io_binding_count = inspector->getProgIOBindings(i, io_bindings, 128);

          if (io_binding_count > 0)
            {
              fprintf(outstream, "    └── Bound I/O: ");

              for (k = 0; k < io_binding_count; k++)
                {
                  if (k > 0)
                    {
                      fprintf(outstream, ", ");
                    }

                  fprintf(outstream, "%s", io_bindings[k]->getName());
                }

              fprintf(outstream, "\n");
            }
        }
    }

  if (has_unbound)
    {
      fprintf(outstream, "\n");
    }

  // Show ConfigIO objects and their bindings

  io_count = inspector->getIOCount();
  has_unbound = false;

  for (i = 0; i < io_count; i++)
    {
      io = inspector->getIO(i);
      if (io == nullptr)
        {
          continue;
        }

      // Check if this is a ConfigIO object

      if (io->getCls() == CIOCommon::IO_CLASS_CONFIG)
        {
          const CIOConfig *cfg_io = static_cast<const CIOConfig *>(io);

          if (!has_unbound)
            {
              fprintf(outstream, "ConfigIO objects:\n");
              has_unbound = true;
            }

          fprintf(outstream, "  * %s\n", io->getName());

          if (!cfg_io->map.empty())
            {
              for (const auto &[id, cfg_bound_obj] : cfg_io->map)
                {
                  if (cfg_bound_obj)
                    {
                      const char *type_str = "UNKNOWN";

                      switch (cfg_bound_obj->getType())
                        {
                          case SObjectId::OBJTYPE_IO:
                            type_str = "IO";
                            break;
                          case SObjectId::OBJTYPE_PROG:
                            type_str = "PROG";
                            break;
                          case SObjectId::OBJTYPE_PROTO:
                            type_str = "PROTO";
                            break;
                        }

                      fprintf(outstream,
                              "    └── Configures %s: %s\n",
                              type_str,
                              cfg_bound_obj->getName());
                    }
                }
            }
        }
    }

  if (has_unbound)
    {
      fprintf(outstream, "\n");
      has_unbound = false;
    }

  // Show unbound IO objects (excluding ConfigIO)

  for (i = 0; i < io_count; i++)
    {
      io = inspector->getIO(i);
      if (io == nullptr)
        {
          continue;
        }

      // Skip ConfigIO objects (already shown above)

      if (io->getCls() == CIOCommon::IO_CLASS_CONFIG)
        {
          continue;
        }

      is_bound = false;

      // Check if bound to PROTO

      for (j = 0; j < proto_count; j++)
        {
          binding_count = inspector->getProtoBindings(j, bound_objs, 128);

          for (k = 0; k < binding_count; k++)
            {
              if (bound_objs[k]->getIdV() == io->getIdV())
                {
                  is_bound = true;
                  break;
                }
            }

          if (is_bound)
            {
              break;
            }
        }

      // Check if bound to PROG

      if (!is_bound)
        {
          for (j = 0; j < prog_count; j++)
            {
              io_binding_count = inspector->getProgIOBindings(j, io_bindings, 128);

              for (k = 0; k < io_binding_count; k++)
                {
                  if (io_bindings[k]->getIdV() == io->getIdV())
                    {
                      is_bound = true;
                      break;
                    }
                }

              if (is_bound)
                {
                  break;
                }
            }
        }

      if (!is_bound)
        {
          if (!has_unbound)
            {
              fprintf(outstream, "Unbound I/O objects:\n");
              has_unbound = true;
            }

          fprintf(outstream, "  * %s\n", io->getName());
        }
    }

  if (has_unbound)
    {
      fprintf(outstream, "\n");
    }
}

void CProtoShellPretty::cmdStats(const char *arg)
{
  CDevInspector *inspector;
  size_t io_count;
#  ifdef CONFIG_DAWN_IO_HAS_STATS
  uint32_t total_reads;
  uint32_t total_writes;
  uint32_t total_errors;
  const CIOCommon *most_reads;
  const CIOCommon *most_writes;
  uint32_t max_reads;
  uint32_t max_writes;
  bool has_errors;
  size_t i;
  const CIOCommon *io;
  const CIOCommon::IOStats *stats;
#  endif

  (void)arg;

  inspector = CDevInspector::getInst();

  fprintf(outstream, "\n");
  fprintf(outstream, "I/O Statistics\n");
  fprintf(outstream, "==============\n\n");

  io_count = inspector->getIOCount();

  if (io_count == 0)
    {
      fprintf(outstream, "No I/O objects registered\n");
      return;
    }

#  ifdef CONFIG_DAWN_IO_HAS_STATS
  // Aggregate statistics

  total_reads = 0;
  total_writes = 0;
  total_errors = 0;

  // Track most active objects

  most_reads = nullptr;
  most_writes = nullptr;
  max_reads = 0;
  max_writes = 0;

  // Process all IO objects

  for (i = 0; i < io_count; i++)
    {
      io = inspector->getIO(i);
      if (io == nullptr)
        {
          continue;
        }

      stats = &io->getStats();
      total_reads += stats->read_count;
      total_writes += stats->write_count;
      total_errors += stats->error_count;

      if (stats->read_count > max_reads)
        {
          max_reads = stats->read_count;
          most_reads = io;
        }

      if (stats->write_count > max_writes)
        {
          max_writes = stats->write_count;
          most_writes = io;
        }
    }

  // Display summary

  fprintf(outstream, "Summary\n");
  fprintf(outstream, "-------\n");
  fprintf(outstream, "  Total I/O Objects: %zu\n", io_count);
  fprintf(outstream, "  Total Reads:       %" PRIu32 "\n", total_reads);
  fprintf(outstream, "  Total Writes:      %" PRIu32 "\n", total_writes);
  fprintf(outstream, "  Total Errors:      %" PRIu32 "\n", total_errors);
  fprintf(outstream, "\n");

  // Display most active objects

  if (most_reads && max_reads > 0)
    {
      fprintf(outstream, "Most Read I/O\n");
      fprintf(outstream, "-------------\n");
      fprintf(outstream, "  Name:         %s\n", most_reads->getName());
      fprintf(outstream, "  Object ID:    0x%08" PRIx32 "\n", most_reads->getIdV());
      fprintf(outstream, "  Read Count:   %" PRIu32 "\n", max_reads);
      fprintf(outstream, "\n");
    }

  if (most_writes && max_writes > 0)
    {
      fprintf(outstream, "Most Written I/O\n");
      fprintf(outstream, "----------------\n");
      fprintf(outstream, "  Name:         %s\n", most_writes->getName());
      fprintf(outstream, "  Object ID:    0x%08" PRIx32 "\n", most_writes->getIdV());
      fprintf(outstream, "  Write Count:  %" PRIu32 "\n", max_writes);
      fprintf(outstream, "\n");
    }

  // Display objects with errors

  has_errors = false;

  for (i = 0; i < io_count; i++)
    {
      io = inspector->getIO(i);
      if (io == nullptr)
        {
          continue;
        }

      stats = &io->getStats();

      if (stats->error_count > 0)
        {
          if (!has_errors)
            {
              fprintf(outstream, "I/O Objects with Errors\n");
              fprintf(outstream, "-----------------------\n");
              has_errors = true;
            }

          fprintf(outstream,
                  "  %-14s 0x%08" PRIx32 " - %" PRIu32 " errors\n",
                  io->getName(),
                  io->getIdV(),
                  stats->error_count);
        }
    }

  if (!has_errors)
    {
      fprintf(outstream, "No errors recorded.\n");
    }

  fprintf(outstream, "\n");
#  else
  fprintf(outstream,
          "Statistics tracking is disabled.\n"
          "Enable CONFIG_DAWN_IO_HAS_STATS to track I/O statistics.\n\n");
#  endif
}

#endif // CONFIG_DAWN_PROTO_SHELL_INSPECT
