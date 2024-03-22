// dawn/src/prog/factory.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/factory.hxx"

#include "dawn/prog/common.hxx"

#ifdef CONFIG_DAWN_PROG_ADJUST
#  include "dawn/prog/adjust.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_DUMMY
#  include "dawn/prog/dummy.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_SAMPLING
#  include "dawn/prog/sampling.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_SEQUENCER
#  include "dawn/prog/sequencer.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_GATEWAY
#  include "dawn/prog/gateway.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_LATEST
#  include "dawn/prog/latest.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_REDIRECT
#  include "dawn/prog/redirect.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_BUFFER
#  include "dawn/prog/buffer.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_MOVING_AVG
#  include "dawn/prog/movingavg.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_IIR_FILTER
#  include "dawn/prog/iirfilter.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_THRESHOLD_ANY
#  include "dawn/prog/threshold.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_AVG
#  include "dawn/prog/statsavg.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_COUNT
#  include "dawn/prog/statscount.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_RMS
#  include "dawn/prog/statsrms.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_MAX
#  include "dawn/prog/statsmax.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_MIN
#  include "dawn/prog/statsmin.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_STATS_SUM
#  include "dawn/prog/statssum.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_BITSPLIT
#  include "dawn/prog/bitsplit.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_TOGGLE
#  include "dawn/prog/toggle.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_COUNTER
#  include "dawn/prog/counter.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_SWITCH
#  include "dawn/prog/switch.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_EXPRESSION
#  include "dawn/prog/expression.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_SELECTOR
#  include "dawn/prog/selector.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_BITPACK
#  include "dawn/prog/bitpack.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_VECPACK
#  include "dawn/prog/vecpack.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_VECSPLIT
#  include "dawn/prog/vecsplit.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_MANYTOONE
#  include "dawn/prog/manytoone.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_ONETOMANY
#  include "dawn/prog/onetomany.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_IOMUX
#  include "dawn/prog/iomux.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_IODEMUX
#  include "dawn/prog/iodemux.hxx"
#endif

#ifdef CONFIG_DAWN_PROG_CONFIGWRITER
#  include "dawn/prog/configwriter.hxx"
#endif

using namespace dawn;

CProgCommon *CProgFactory::create(CDescObject &desc)
{
  DEBUGASSERT(desc.getObjectType() == SObjectId::OBJTYPE_PROG);

  switch (desc.getObjectId().s.cls)
    {
#ifdef CONFIG_DAWN_PROG_ADJUST
      case CProgCommon::PROG_CLASS_ADJUST:
        return new CProgAdjust(desc);
#endif

#ifdef CONFIG_DAWN_PROG_DUMMY
      case CProgCommon::PROG_CLASS_DUMMY:
        return new CProgDummy(desc);
#endif

#ifdef CONFIG_DAWN_PROG_SAMPLING
      case CProgCommon::PROG_CLASS_SAMPLING:
        return new CProgSampling(desc);
#endif

#ifdef CONFIG_DAWN_PROG_SEQUENCER
      case CProgCommon::PROG_CLASS_SEQUENCER:
        return new CProgSequencer(desc);
#endif

#ifdef CONFIG_DAWN_PROG_GATEWAY
      case CProgCommon::PROG_CLASS_GATEWAY:
        return new CProgGateway(desc);
#endif

#ifdef CONFIG_DAWN_PROG_LATEST
      case CProgCommon::PROG_CLASS_LATEST:
        return new CProgLatest(desc);
#endif

#ifdef CONFIG_DAWN_PROG_REDIRECT
      case CProgCommon::PROG_CLASS_REDIRECT:
        return new CProgRedirect(desc);
#endif

#ifdef CONFIG_DAWN_PROG_BUFFER
      case CProgCommon::PROG_CLASS_BUFFER:
        return new CProgBuffer(desc);
#endif

#ifdef CONFIG_DAWN_PROG_MOVING_AVG
      case CProgCommon::PROG_CLASS_MOVING_AVG:
        return new CProgMovingAverage(desc);
#endif

#ifdef CONFIG_DAWN_PROG_IIR_FILTER
      case CProgCommon::PROG_CLASS_IIR_FILTER:
        return new CProgIIRFilter(desc);
#endif

#ifdef CONFIG_DAWN_PROG_THRESHOLD
      case CProgCommon::PROG_CLASS_THRESHOLD:
        return new CProgThreshold(desc);
#endif

#ifdef CONFIG_DAWN_PROG_THRESHOLD_VALUE
      case CProgCommon::PROG_CLASS_THRESHOLD_VALUE:
        return new CProgThresholdValue(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_MIN
      case CProgCommon::PROG_CLASS_STATS_MIN:
        return new CProgStatsMin(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_MAX
      case CProgCommon::PROG_CLASS_STATS_MAX:
        return new CProgStatsMax(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_AVG
      case CProgCommon::PROG_CLASS_STATS_AVG:
        return new CProgStatsAvg(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_SUM
      case CProgCommon::PROG_CLASS_STATS_SUM:
        return new CProgStatsSum(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_COUNT
      case CProgCommon::PROG_CLASS_STATS_COUNT:
        return new CProgStatsCount(desc);
#endif

#ifdef CONFIG_DAWN_PROG_STATS_RMS
      case CProgCommon::PROG_CLASS_STATS_RMS:
        return new CProgStatsRms(desc);
#endif

#ifdef CONFIG_DAWN_PROG_BITSPLIT
      case CProgCommon::PROG_CLASS_BITSPLIT:
        return new CProgBitSplit(desc);
#endif

#ifdef CONFIG_DAWN_PROG_TOGGLE
      case CProgCommon::PROG_CLASS_TOGGLE:
        return new CProgToggle(desc);
#endif

#ifdef CONFIG_DAWN_PROG_COUNTER
      case CProgCommon::PROG_CLASS_COUNTER:
        return new CProgCounter(desc);
#endif

#ifdef CONFIG_DAWN_PROG_SWITCH
      case CProgCommon::PROG_CLASS_SWITCH:
        return new CProgSwitch(desc);
#endif

#ifdef CONFIG_DAWN_PROG_EXPRESSION
      case CProgCommon::PROG_CLASS_EXPRESSION:
        return new CProgExpression(desc);
#endif

#ifdef CONFIG_DAWN_PROG_SELECTOR
      case CProgCommon::PROG_CLASS_SELECTOR:
        return new CProgSelector(desc);
#endif

#ifdef CONFIG_DAWN_PROG_BITPACK
      case CProgCommon::PROG_CLASS_BITPACK:
        return new CProgBitPack(desc);
#endif

#ifdef CONFIG_DAWN_PROG_VECPACK
      case CProgCommon::PROG_CLASS_VECPACK:
        return new CProgVecPack(desc);
#endif

#ifdef CONFIG_DAWN_PROG_VECSPLIT
      case CProgCommon::PROG_CLASS_VECSPLIT:
        return new CProgVecSplit(desc);
#endif

#ifdef CONFIG_DAWN_PROG_MANYTOONE
      case CProgCommon::PROG_CLASS_MANYTOONE:
        return new CProgManyToOne(desc);
#endif

#ifdef CONFIG_DAWN_PROG_ONETOMANY
      case CProgCommon::PROG_CLASS_ONETOMANY:
        return new CProgOneToMany(desc);
#endif

#ifdef CONFIG_DAWN_PROG_IOMUX
      case CProgCommon::PROG_CLASS_IOMUX:
        return new CProgIOMux(desc);
#endif

#ifdef CONFIG_DAWN_PROG_IODEMUX
      case CProgCommon::PROG_CLASS_IODEMUX:
        return new CProgIODemux(desc);
#endif

#ifdef CONFIG_DAWN_PROG_CONFIGWRITER
      case CProgCommon::PROG_CLASS_CONFIGWRITER:
        return new CProgConfigWriter(desc);
#endif

      default:
        {
          DAWNERR("Unknown PROG class %d\n", desc.getObjectCls());
          return nullptr;
        }
    }
}
