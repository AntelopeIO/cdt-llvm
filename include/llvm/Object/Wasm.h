//===- Wasm.h - Wasm object file implementation -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the WasmObjectFile class, which implements the ObjectFile
// interface for Wasm files.
//
// See: https://github.com/WebAssembly/design/blob/master/BinaryEncoding.md
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_OBJECT_WASM_H
#define LLVM_OBJECT_WASM_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Wasm.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/MC/MCSymbolWasm.h"
#include "llvm/Object/Binary.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace llvm {
namespace object {

class WasmSymbol {
public:
  WasmSymbol(const wasm::WasmSymbolInfo &Info,
             const wasm::WasmGlobalType *GlobalType,
             const wasm::WasmEventType *EventType,
             const wasm::WasmSignature *Signature)
      : Info(Info), GlobalType(GlobalType), EventType(EventType),
        Signature(Signature) {}

  const wasm::WasmSymbolInfo &Info;
  const wasm::WasmGlobalType *GlobalType;
  const wasm::WasmEventType *EventType;
  const wasm::WasmSignature *Signature;

  bool isTypeFunction() const {
    return Info.Kind == wasm::WASM_SYMBOL_TYPE_FUNCTION;
  }

  bool isTypeData() const { return Info.Kind == wasm::WASM_SYMBOL_TYPE_DATA; }

  bool isTypeGlobal() const {
    return Info.Kind == wasm::WASM_SYMBOL_TYPE_GLOBAL;
  }

  bool isTypeSection() const {
    return Info.Kind == wasm::WASM_SYMBOL_TYPE_SECTION;
  }

  bool isTypeEvent() const { return Info.Kind == wasm::WASM_SYMBOL_TYPE_EVENT; }

  bool isDefined() const { return !isUndefined(); }

  bool isUndefined() const {
    return (Info.Flags & wasm::WASM_SYMBOL_UNDEFINED) != 0;
  }

  bool isBindingWeak() const {
    return getBinding() == wasm::WASM_SYMBOL_BINDING_WEAK;
  }

  bool isBindingGlobal() const {
    return getBinding() == wasm::WASM_SYMBOL_BINDING_GLOBAL;
  }

  bool isBindingLocal() const {
    return getBinding() == wasm::WASM_SYMBOL_BINDING_LOCAL;
  }

  unsigned getBinding() const {
    return Info.Flags & wasm::WASM_SYMBOL_BINDING_MASK;
  }

  bool isHidden() const {
    return getVisibility() == wasm::WASM_SYMBOL_VISIBILITY_HIDDEN;
  }

  unsigned getVisibility() const {
    return Info.Flags & wasm::WASM_SYMBOL_VISIBILITY_MASK;
  }

  void print(raw_ostream &Out) const;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
  LLVM_DUMP_METHOD void dump() const;
#endif
};

struct WasmSection {
  WasmSection() = default;

  uint32_t Type = 0;         // Section type (See below)
  uint32_t Offset = 0;       // Offset with in the file
  StringRef Name;            // Section name (User-defined sections only)
  ArrayRef<uint8_t> Content; // Section content
  std::vector<wasm::WasmRelocation> Relocations; // Relocations for this section
};

struct WasmSegment {
  uint32_t SectionOffset;
  wasm::WasmDataSegment Data;
};

class WasmObjectFile : public ObjectFile {

public:
  WasmObjectFile(MemoryBufferRef Object, Error &Err);

  const wasm::WasmObjectHeader &getHeader() const;
  const WasmSymbol &getWasmSymbol(const DataRefImpl &Symb) const;
  const WasmSymbol &getWasmSymbol(const SymbolRef &Symbol) const;
  const WasmSection &getWasmSection(const SectionRef &Section) const;
  const wasm::WasmRelocation &getWasmRelocation(const RelocationRef &Ref) const;

  static bool classof(const Binary *v) { return v->isWasm(); }

  const wasm::WasmDylinkInfo &dylinkInfo() const { return DylinkInfo; }
  const wasm::WasmProducerInfo &getProducerInfo() const { return ProducerInfo; }
  ArrayRef<wasm::WasmFeatureEntry> getTargetFeatures() const {
    return TargetFeatures;
  }
  ArrayRef<wasm::WasmSignature> types() const { return Signatures; }
  ArrayRef<uint32_t> functionTypes() const { return FunctionTypes; }
  ArrayRef<wasm::WasmImport> imports() const { return Imports; }
  ArrayRef<wasm::WasmTable> tables() const { return Tables; }
  ArrayRef<wasm::WasmLimits> memories() const { return Memories; }
  ArrayRef<wasm::WasmGlobal> globals() const { return Globals; }
  ArrayRef<wasm::WasmEvent> events() const { return Events; }
  ArrayRef<wasm::WasmExport> exports() const { return Exports; }
  ArrayRef<WasmSymbol> syms() const { return Symbols; }
  const wasm::WasmLinkingData &linkingData() const { return LinkingData; }
  uint32_t getNumberOfSymbols() const { return Symbols.size(); }
  ArrayRef<wasm::WasmElemSegment> elements() const { return ElemSegments; }
  ArrayRef<WasmSegment> dataSegments() const { return DataSegments; }
  ArrayRef<wasm::WasmFunction> functions() const { return Functions; }
  ArrayRef<wasm::WasmFunctionName> debugNames() const { return DebugNames; }
  ArrayRef<StringRef> allowed_imports() const { return AllowedImports; }
  ArrayRef<StringRef> actions() const { return Actions; }
  ArrayRef<StringRef> calls() const { return Calls; }
  ArrayRef<StringRef> notify() const { return Notify; }
  StringRef get_eosio_abi() const { return eosio_abi; }
  uint32_t startFunction() const { return StartFunction; }
  uint32_t getNumImportedGlobals() const { return NumImportedGlobals; }
  uint32_t getNumImportedFunctions() const { return NumImportedFunctions; }
  uint32_t getNumImportedEvents() const { return NumImportedEvents; }
  void moveSymbolNext(DataRefImpl &Symb) const override;

  uint32_t getSymbolFlags(DataRefImpl Symb) const override;

  basic_symbol_iterator symbol_begin() const override;

  basic_symbol_iterator symbol_end() const override;
  Expected<StringRef> getSymbolName(DataRefImpl Symb) const override;

  Expected<uint64_t> getSymbolAddress(DataRefImpl Symb) const override;
  uint64_t getWasmSymbolValue(const WasmSymbol &Sym) const;
  uint64_t getSymbolValueImpl(DataRefImpl Symb) const override;
  uint32_t getSymbolAlignment(DataRefImpl Symb) const override;
  uint64_t getCommonSymbolSizeImpl(DataRefImpl Symb) const override;
  Expected<SymbolRef::Type> getSymbolType(DataRefImpl Symb) const override;
  Expected<section_iterator> getSymbolSection(DataRefImpl Symb) const override;

  // Overrides from SectionRef.
  void moveSectionNext(DataRefImpl &Sec) const override;
  Expected<StringRef> getSectionName(DataRefImpl Sec) const override;
  uint64_t getSectionAddress(DataRefImpl Sec) const override;
  uint64_t getSectionIndex(DataRefImpl Sec) const override;
  uint64_t getSectionSize(DataRefImpl Sec) const override;
  Expected<ArrayRef<uint8_t>>
  getSectionContents(DataRefImpl Sec) const override;
  uint64_t getSectionAlignment(DataRefImpl Sec) const override;
  bool isSectionCompressed(DataRefImpl Sec) const override;
  bool isSectionText(DataRefImpl Sec) const override;
  bool isSectionData(DataRefImpl Sec) const override;
  bool isSectionBSS(DataRefImpl Sec) const override;
  bool isSectionVirtual(DataRefImpl Sec) const override;
  bool isSectionBitcode(DataRefImpl Sec) const override;
  relocation_iterator section_rel_begin(DataRefImpl Sec) const override;
  relocation_iterator section_rel_end(DataRefImpl Sec) const override;

  // Overrides from RelocationRef.
  void moveRelocationNext(DataRefImpl &Rel) const override;
  uint64_t getRelocationOffset(DataRefImpl Rel) const override;
  symbol_iterator getRelocationSymbol(DataRefImpl Rel) const override;
  uint64_t getRelocationType(DataRefImpl Rel) const override;
  void getRelocationTypeName(DataRefImpl Rel,
                             SmallVectorImpl<char> &Result) const override;

  section_iterator section_begin() const override;
  section_iterator section_end() const override;
  uint8_t getBytesInAddress() const override;
  StringRef getFileFormatName() const override;
  Triple::ArchType getArch() const override;
  SubtargetFeatures getFeatures() const override;
  bool isRelocatableObject() const override;
  bool isSharedObject() const;

  struct ReadContext {
    const uint8_t *Start;
    const uint8_t *Ptr;
    const uint8_t *End;
  };

private:
  bool isValidFunctionIndex(uint32_t Index) const;
  bool isDefinedFunctionIndex(uint32_t Index) const;
  bool isValidGlobalIndex(uint32_t Index) const;
  bool isDefinedGlobalIndex(uint32_t Index) const;
  bool isValidEventIndex(uint32_t Index) const;
  bool isDefinedEventIndex(uint32_t Index) const;
  bool isValidFunctionSymbol(uint32_t Index) const;
  bool isValidGlobalSymbol(uint32_t Index) const;
  bool isValidEventSymbol(uint32_t Index) const;
  bool isValidDataSymbol(uint32_t Index) const;
  bool isValidSectionSymbol(uint32_t Index) const;
  wasm::WasmFunction &getDefinedFunction(uint32_t Index);
  const wasm::WasmFunction &getDefinedFunction(uint32_t Index) const;
  wasm::WasmGlobal &getDefinedGlobal(uint32_t Index);
  wasm::WasmEvent &getDefinedEvent(uint32_t Index);

  const WasmSection &getWasmSection(DataRefImpl Ref) const;
  const wasm::WasmRelocation &getWasmRelocation(DataRefImpl Ref) const;

  Error parseSection(WasmSection &Sec);
  Error parseCustomSection(WasmSection &Sec, ReadContext &Ctx);

  // Standard section types
  Error parseTypeSection(ReadContext &Ctx);
  Error parseImportSection(ReadContext &Ctx);
  Error parseFunctionSection(ReadContext &Ctx);
  Error parseTableSection(ReadContext &Ctx);
  Error parseMemorySection(ReadContext &Ctx);
  Error parseGlobalSection(ReadContext &Ctx);
  Error parseEventSection(ReadContext &Ctx);
  Error parseExportSection(ReadContext &Ctx);
  Error parseStartSection(ReadContext &Ctx);
  Error parseElemSection(ReadContext &Ctx);
  Error parseCodeSection(ReadContext &Ctx);
  Error parseDataSection(ReadContext &Ctx);
  Error parseDataCountSection(ReadContext &Ctx);

  // Custom section types
  Error parseDylinkSection(ReadContext &Ctx);
  Error parseNameSection(ReadContext &Ctx);
  Error parseAllowedSection(ReadContext &Ctx);
  Error parseActionsSection(ReadContext &Ctx);
  Error parseCallsSection(ReadContext &Ctx);
  Error parseNotifySection(ReadContext &Ctx);
  Error parseLinkingSection(ReadContext &Ctx);
  Error parseLinkingSectionSymtab(ReadContext &Ctx);
  Error parseLinkingSectionComdat(ReadContext &Ctx);
  Error parseProducersSection(ReadContext &Ctx);
  Error parseTargetFeaturesSection(ReadContext &Ctx);
  Error parseRelocSection(StringRef Name, ReadContext &Ctx);
  Error parseEosioABISection(ReadContext &Ctx);

  wasm::WasmObjectHeader Header;
  std::vector<WasmSection> Sections;
  wasm::WasmDylinkInfo DylinkInfo;
  wasm::WasmProducerInfo ProducerInfo;
  std::vector<wasm::WasmFeatureEntry> TargetFeatures;
  std::vector<wasm::WasmSignature> Signatures;
  std::vector<uint32_t> FunctionTypes;
  std::vector<wasm::WasmTable> Tables;
  std::vector<wasm::WasmLimits> Memories;
  std::vector<wasm::WasmGlobal> Globals;
  std::vector<wasm::WasmEvent> Events;
  std::vector<wasm::WasmImport> Imports;
  std::vector<StringRef> AllowedImports;
  std::vector<StringRef> Actions;
  std::vector<StringRef> Calls;
  std::vector<StringRef> Notify;
  std::vector<wasm::WasmExport> Exports;
  std::vector<wasm::WasmElemSegment> ElemSegments;
  std::vector<WasmSegment> DataSegments;
  llvm::Optional<size_t> DataCount;
  std::vector<wasm::WasmFunction> Functions;
  std::vector<WasmSymbol> Symbols;
  std::vector<wasm::WasmFunctionName> DebugNames;
  StringRef eosio_abi;
  uint32_t StartFunction = -1;
  bool HasLinkingSection = false;
  bool HasDylinkSection = false;
  wasm::WasmLinkingData LinkingData;
  uint32_t NumImportedGlobals = 0;
  uint32_t NumImportedFunctions = 0;
  uint32_t NumImportedEvents = 0;
  uint32_t CodeSection = 0;
  uint32_t DataSection = 0;
  uint32_t GlobalSection = 0;
  uint32_t EventSection = 0;
};

class WasmSectionOrderChecker {
public:
  // We define orders for all core wasm sections and known custom sections.
  enum : int {
    // Sentinel, must be zero
    WASM_SEC_ORDER_NONE = 0,

    // Core sections
    WASM_SEC_ORDER_TYPE,
    WASM_SEC_ORDER_IMPORT,
    WASM_SEC_ORDER_FUNCTION,
    WASM_SEC_ORDER_TABLE,
    WASM_SEC_ORDER_MEMORY,
    WASM_SEC_ORDER_GLOBAL,
    WASM_SEC_ORDER_EVENT,
    WASM_SEC_ORDER_EXPORT,
    WASM_SEC_ORDER_START,
    WASM_SEC_ORDER_ELEM,
    WASM_SEC_ORDER_DATACOUNT,
    WASM_SEC_ORDER_CODE,
    WASM_SEC_ORDER_DATA,

    // Custom sections
    // "dylink" should be the very first section in the module
    WASM_SEC_ORDER_DYLINK,
    // "linking" section requires DATA section in order to validate data symbols
    WASM_SEC_ORDER_LINKING,
    // Must come after "linking" section in order to validate reloc indexes.
    WASM_SEC_ORDER_RELOC,
    // "name" section must appear after DATA. Comes after "linking" to allow
    // symbol table to set default function name.
    WASM_SEC_ORDER_NAME,
    // "producers" section must appear after "name" section.
    WASM_SEC_ORDER_PRODUCERS,
    // "target_features" section must appear after producers section
    WASM_SEC_ORDER_TARGET_FEATURES,

    // Must be last
    WASM_NUM_SEC_ORDERS

  };

  // Sections that may or may not be present, but cannot be predecessors
  static int DisallowedPredecessors[WASM_NUM_SEC_ORDERS][WASM_NUM_SEC_ORDERS];

  bool isValidSectionOrder(unsigned ID, StringRef CustomSectionName = "");

private:
  bool Seen[WASM_NUM_SEC_ORDERS] = {}; // Sections that have been seen already

  // Returns -1 for unknown sections.
  int getSectionOrder(unsigned ID, StringRef CustomSectionName = "");
};

} // end namespace object

inline raw_ostream &operator<<(raw_ostream &OS, const object::WasmSymbol &Sym) {
  Sym.print(OS);
  return OS;
}

} // end namespace llvm

#endif // LLVM_OBJECT_WASM_H
