//===-- M88kAsmParser.cpp - Parse M88k assembly to MCInst instructions ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/M88kInstPrinter.h"
#include "MCTargetDesc/M88kMCExpr.h"
#include "MCTargetDesc/M88kMCTargetDesc.h"
#include "MCTargetDesc/M88kTargetStreamer.h"
#include "TargetInfo/M88kTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCAsmMacro.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/TargetParser/Triple.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

using namespace llvm;

namespace {

// Return true if Expr is in the range [MinValue, MaxValue].
bool inRange(const MCExpr *Expr, int64_t MinValue, int64_t MaxValue) {
  if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    int64_t Value = CE->getValue();
    return Value >= MinValue && Value <= MaxValue;
  }
  return false;
}

// Name of the sentinel token that replaces the operand list after the parser
// has handled a GNU-style undotted directive (see parseGNUDirective).
const char *const GNUDirectiveSentinel = "<gnu-directive>";

// Condition names accepted by GNU as for bb0/bb1 and inside <...> bitfield
// offsets.  Returns -1 if Name is not a condition name.
int getGNUConditionName(StringRef Name) {
  return StringSwitch<int>(Name)
      .Case("nc", 0)
      .Case("cp", 1)
      .Case("eq", 2)
      .Case("ne", 3)
      .Case("gt", 4)
      .Case("le", 5)
      .Case("lt", 6)
      .Case("ge", 7)
      .Case("hi", 8)
      .Case("ou", 8)
      .Case("ls", 9)
      .Case("ib", 9)
      .Case("lo", 10)
      .Case("in", 10)
      .Case("hs", 11)
      .Case("ob", 11)
      .Case("be", 12)
      .Case("nb", 13)
      .Case("he", 14)
      .Case("nh", 15)
      .Default(-1);
}

// Instances of this class represented a parsed machine instruction
class M88kOperand : public MCParsedAsmOperand {
  enum OperandKind {
    // KindInvalid,
    OpKind_Token,
    OpKind_Reg,
    OpKind_Imm,
  };

  OperandKind Kind;
  SMLoc StartLoc, EndLoc;

  struct TokenOp {
    StringRef Token;
  };

  struct RegOp {
    unsigned RegNo;
  };

  union {
    TokenOp Tok;
    RegOp Reg;
    const MCExpr *Imm;
  };

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    // Add as immediates when possible.  Null MCExpr = 0.
    if (!Expr)
      Inst.addOperand(MCOperand::createImm(0));
    else if (auto *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

public:
  M88kOperand(OperandKind Kind, SMLoc StartLoc, SMLoc EndLoc)
      : Kind(Kind), StartLoc(StartLoc), EndLoc(EndLoc) {}

  // getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }

  // getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }

  bool isReg() const override { return Kind == OpKind_Reg; }

  MCRegister getReg() const override {
    assert(isReg() && "Invalid type access!");
    return Reg.RegNo;
  }

  bool isImm() const override { return Kind == OpKind_Imm; }

  bool isImm(int64_t MinValue, int64_t MaxValue) const {
    return Kind == OpKind_Imm && inRange(Imm, MinValue, MaxValue);
  }

  const MCExpr *getImm() const {
    assert(isImm() && "Invalid type access!");
    return Imm;
  }

  bool isToken() const override { return Kind == OpKind_Token; }

  StringRef getToken() const {
    assert(isToken() && "Not a token");
    return Tok.Token;
  }

  bool isMem() const override { return false; }

  static std::unique_ptr<M88kOperand> createToken(StringRef Str, SMLoc Loc) {
    auto Op = std::make_unique<M88kOperand>(OpKind_Token, Loc, Loc);
    Op->Tok.Token = Str;
    return Op;
  }

  static std::unique_ptr<M88kOperand> createReg(unsigned Num, SMLoc StartLoc,
                                                SMLoc EndLoc) {
    auto Op = std::make_unique<M88kOperand>(OpKind_Reg, StartLoc, EndLoc);
    Op->Reg.RegNo = Num;
    return Op;
  }

  static std::unique_ptr<M88kOperand> createImm(const MCExpr *Expr,
                                                SMLoc StartLoc, SMLoc EndLoc) {
    auto Op = std::make_unique<M88kOperand>(OpKind_Imm, StartLoc, EndLoc);
    Op->Imm = Expr;
    return Op;
  }

  // Used by the TableGen code to add particular types of operand
  // to an instruction.
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    addExpr(Inst, getImm());
  }

  void addBFWidthOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    addExpr(Inst, getImm());
  }

  void addBFOffsetOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    addExpr(Inst, getImm());
  }

  void addPixelRotOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    addExpr(Inst, getImm());
  }

  void addConditionCodeOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands");
    addExpr(Inst, getImm());
  }

  // A symbolic 16-bit immediate: a relocation specifier such as
  // %hi16(sym) / %lo16(sym), or any other non-constant expression that will
  // be resolved by a relocation.  Plain constants are handled by isImm().
  bool isSymbolic16() const {
    return Kind == OpKind_Imm && Imm && !isa<MCConstantExpr>(Imm);
  }

  bool isU5Imm() const { return isImm(0, 31); }
  bool isU16Imm() const { return isImm(0, 65535) || isSymbolic16(); }
  bool isS16Imm() const { return isImm(-32768, 32767) || isSymbolic16(); }
  bool isVec9() const { return isImm(0, 511); }

  bool isBFWidth() const { return isImm(0, 31); }
  bool isBFOffset() const { return isImm(0, 31); }
  bool isPixelRot() const { return isImm(0, 60); }
  bool isCCode() const { return isImm(0, 31); }

  void print(raw_ostream &OS, const MCAsmInfo &) const override {
    switch (Kind) {
    case OpKind_Imm:
      OS << "Imm: " << getImm() << "\n";
      break;
    case OpKind_Token:
      OS << "Token: " << getToken() << "\n";
      break;
    case OpKind_Reg:
      OS << "Reg: " << M88kInstPrinter::getRegisterName(getReg()) << "\n";
      break;
    }
  }
};

class M88kAsmParser : public MCTargetAsmParser {
// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "M88kGenAsmMatcher.inc"

  M88kTargetStreamer &getTargetStreamer() {
    assert(getParser().getStreamer().getTargetStreamer() &&
           "m88k - asm parser does not have a target streamer");
    MCTargetStreamer &TS = *getParser().getStreamer().getTargetStreamer();
    return static_cast<M88kTargetStreamer &>(TS);
  }

  bool ParseDirective(AsmToken DirectiveID) override;
  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool parseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  ParseStatus tryParseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                               SMLoc &EndLoc) override;
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

  bool parseRegister(MCRegister &RegNo, SMLoc &StartLoc, SMLoc &EndLoc,
                     bool RestoreOnFailure);
  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);
  bool parseScaledRegister(OperandVector &Operands);

  // GNU mode: handle undotted directives (word, text, data, align, space,
  // string).  Sets Handled if Name was such a directive.  Returns true on
  // error.
  bool parseGNUDirective(StringRef Name, SMLoc NameLoc, bool &Handled);

  // Parse a %hi16(...) / %lo16(...) relocation operand.  On entry the lexer
  // is positioned on the identifier following the '%'.  Consumes the
  // identifier, the parenthesised sub-expression, and pushes an immediate
  // operand wrapping an M88kMCExpr.  Returns true on error.
  bool parseRelocExpr(OperandVector &Operands, SMLoc PercentLoc);

  ParseStatus parseBFWidth(OperandVector &Operands);
  ParseStatus parseBFOffset(OperandVector &Operands);
  ParseStatus parsePixelRot(OperandVector &Operands);
  ParseStatus parseConditionCode(OperandVector &Operands);

  ParseStatus parsePCRel(OperandVector &Operands, unsigned Bits);

  ParseStatus parsePCRel16(OperandVector &Operands) {
    return parsePCRel(Operands, 18);
  }

  ParseStatus parsePCRel26(OperandVector &Operands) {
    return parsePCRel(Operands, 28);
  }

  bool matchAndEmitInstruction(SMLoc IdLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

public:
  enum M88kMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "M88kGenAsmMatcher.inc"
  };

  M88kAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser),
        Lexer(Parser.getLexer()), SubtargetInfo(STI) {
    // GNU as syntax is enabled by -mattr=+gnu-asm and implicitly for NetBSD.
    GNUMode = STI.hasFeature(M88k::FeatureGNUAsm) ||
              STI.getTargetTriple().isOSNetBSD();
    setAvailableFeatures(
        ComputeAvailableFeatures(SubtargetInfo.getFeatureBits()));
  }

private:
  MCAsmParser &Parser;
  AsmLexer &Lexer;

  const MCSubtargetInfo &SubtargetInfo;

  // True if GNU as syntax is accepted in addition to the %-prefixed syntax.
  bool GNUMode = false;
};

} // end anonymous namespace

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "M88kGenAsmMatcher.inc"

// Matches the normal register name, or the alternative register name.
static unsigned matchRegisterName(StringRef Name) {
  unsigned RegNo = MatchRegisterName(Name);
  if (RegNo == 0)
    RegNo = MatchRegisterAltName(Name);
  return RegNo;
}

unsigned M88kAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                   unsigned Kind) {
  if (Kind == MCK_GPR64 && AsmOp.isReg()) {
    switch (AsmOp.getReg()) {
    case M88k::R0:
    case M88k::R2:
    case M88k::R4:
    case M88k::R6:
    case M88k::R8:
    case M88k::R10:
    case M88k::R12:
    case M88k::R14:
    case M88k::R16:
    case M88k::R18:
    case M88k::R20:
    case M88k::R22:
    case M88k::R24:
    case M88k::R26:
    case M88k::R28:
    case M88k::R30:
      return Match_Success;
    case M88k::R1:
    case M88k::R3:
    case M88k::R5:
    case M88k::R7:
    case M88k::R9:
    case M88k::R11:
    case M88k::R13:
    case M88k::R15:
    case M88k::R17:
    case M88k::R19:
    case M88k::R21:
    case M88k::R23:
    case M88k::R25:
    case M88k::R27:
    case M88k::R29:
    case M88k::R31:
      // TODO Add option to flag odd registers.
      return Match_Success;
    default:
      return Match_InvalidOperand;
    }
  }

  return Match_InvalidOperand;
}

bool M88kAsmParser::ParseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();

  if (IDVal == ".requires_88110") {
    MCSubtargetInfo &STI = copySTI();
    STI.setDefaultFeatures(/*CPU*/ "mc88110", /*TuneCPU*/ "mc88110", "");
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
    getTargetStreamer().emitDirectiveRequires881100();
    return false;
  }

  return true;
}

bool M88kAsmParser::parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                     SMLoc NameLoc, OperandVector &Operands) {
  if (GNUMode) {
    bool Handled = false;
    if (parseGNUDirective(Name, NameLoc, Handled))
      return true;
    if (Handled) {
      Operands.push_back(M88kOperand::createToken(GNUDirectiveSentinel, NameLoc));
      return false;
    }
  }

  // First operand in MCInst is instruction mnemonic.
  Operands.push_back(M88kOperand::createToken(Name, NameLoc));

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {

    // Read the first operand.
    if (parseOperand(Operands, Name)) {
      return Error(getLexer().getLoc(), "expected operand");
    }

    // Read the second operand.
    if (getLexer().is(AsmToken::Comma)) {
      Parser.Lex();
      if (parseOperand(Operands, Name)) {
        return Error(getLexer().getLoc(), "expected operand");
      }

      // Read the third operand or a scaled register.
      if (getLexer().is(AsmToken::Comma)) {
        Parser.Lex();
        // GNU as accepts "rot rd,rs,0<o>" as well as "rot rd,rs,<o>".
        if (GNUMode && Name == "rot" && getLexer().is(AsmToken::Integer) &&
            getLexer().peekTok().is(AsmToken::Less)) {
          if (getLexer().getTok().getIntVal() != 0)
            return Error(getLexer().getLoc(), "rot width must be 0");
          Parser.Lex();
        }
        if (getLexer().is(AsmToken::Less) && Name == "rot")
          Operands.push_back(
              M88kOperand::createToken("<", Parser.getTok().getLoc()));

        if (parseOperand(Operands, Name)) {
          return Error(getLexer().getLoc(), "expected register or immediate");
        }
        // Parse bitfield width
        if (getLexer().is(AsmToken::Less)) {
          Operands.push_back(
              M88kOperand::createToken("<", Parser.getTok().getLoc()));
          if (parseOperand(Operands, Name)) {
            return Error(getLexer().getLoc(), "expected bitfield offset");
          }
        }
      } else if (getLexer().is(AsmToken::LBrac)) {
        if (parseScaledRegister(Operands))
          return Error(getLexer().getLoc(), "expected scaled register operand");
      }
    }

    if (getLexer().isNot(AsmToken::EndOfStatement))
      return Error(getLexer().getLoc(), "unexpected token in argument list");
  }

  // Consume the EndOfStatement.
  Parser.Lex();
  return false;
}

bool M88kAsmParser::parseGNUDirective(StringRef Name, SMLoc NameLoc,
                                      bool &Handled) {
  enum { None, Word, Text, Data, AlignDir, Space, String } Kind =
      StringSwitch<decltype(None)>(Name)
          .Case("word", Word)
          .Case("text", Text)
          .Case("data", Data)
          .Case("align", AlignDir)
          .Case("space", Space)
          .Case("string", String)
          .Default(None);
  if (Kind == None)
    return false;
  Handled = true;

  MCStreamer &Out = Parser.getStreamer();
  auto AtEnd = [&] { return Lexer.is(AsmToken::EndOfStatement); };

  switch (Kind) {
  case Word:
    while (!AtEnd()) {
      const MCExpr *Expr;
      if (Parser.parseExpression(Expr))
        return true;
      Out.emitValue(Expr, 4, NameLoc);
      if (Lexer.isNot(AsmToken::Comma))
        break;
      Parser.Lex();
    }
    break;
  case Text:
  case Data: {
    // An optional subsection number is accepted and ignored.
    const MCExpr *Expr;
    if (!AtEnd() && Parser.parseExpression(Expr))
      return true;
    const MCObjectFileInfo *OFI = getContext().getObjectFileInfo();
    Out.switchSection(Kind == Text ? OFI->getTextSection()
                                   : OFI->getDataSection());
    break;
  }
  case AlignDir: {
    // GNU as for m88k counts bytes, not powers of two (as does the generic
    // ".align" directive for this target).
    SMLoc ValLoc = Lexer.getLoc();
    int64_t Alignment;
    if (Parser.parseAbsoluteExpression(Alignment))
      return true;
    if (Alignment <= 0 || !isPowerOf2_64(Alignment))
      return Error(ValLoc, "alignment must be a power of 2");
    bool HasFill = false;
    int64_t Fill = 0;
    if (Lexer.is(AsmToken::Comma)) {
      Parser.Lex();
      if (Parser.parseAbsoluteExpression(Fill))
        return true;
      HasFill = true;
    }
    if (!HasFill && getContext().getAsmInfo()->useCodeAlign(*Out.getCurrentSectionOnly()))
      Out.emitCodeAlignment(Align(Alignment), &getSTI());
    else
      Out.emitValueToAlignment(Align(Alignment), Fill, 1);
    break;
  }
  case Space: {
    SMLoc ValLoc = Lexer.getLoc();
    int64_t Size;
    if (Parser.parseAbsoluteExpression(Size))
      return true;
    if (Size < 0)
      return Error(ValLoc, "invalid space size");
    int64_t Fill = 0;
    if (Lexer.is(AsmToken::Comma)) {
      Parser.Lex();
      if (Parser.parseAbsoluteExpression(Fill))
        return true;
    }
    Out.emitFill(*MCConstantExpr::create(Size, getContext()),
                 static_cast<uint8_t>(Fill), NameLoc);
    break;
  }
  case String:
    while (!AtEnd()) {
      std::string Str;
      if (Parser.parseEscapedString(Str))
        return true;
      // GNU as for m88k does not append a NUL byte (unlike ".string").
      Out.emitBytes(Str);
      if (Lexer.isNot(AsmToken::Comma))
        break;
      Parser.Lex();
    }
    break;
  case None:
    break;
  }

  if (Lexer.isNot(AsmToken::EndOfStatement))
    return Error(Lexer.getLoc(), "unexpected token in directive");
  Parser.Lex(); // Consume the EndOfStatement.
  return false;
}

bool M88kAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic) {
  // Invoke a custom associated parser.
  ParseStatus Result = MatchOperandParserImpl(Operands, Mnemonic);

  if (Result.isSuccess()) {
    return false;
  }
  if (Result.isFailure()) {
    Parser.eatToEndOfStatement();
    return true;
  }
  assert(Result.isNoMatch() && "Unexpected match result");

  // A '%' introduces either a register (%r2) or a relocation specifier
  // (%hi16(sym) / %lo16(sym)).  Distinguish by peeking at the identifier.
  if (Lexer.is(AsmToken::Percent)) {
    SMLoc PercentLoc = Parser.getTok().getLoc();
    const AsmToken &Next = Lexer.peekTok();
    if (Next.is(AsmToken::Identifier)) {
      StringRef Id = Next.getIdentifier();
      if (Id == "hi16" || Id == "lo16") {
        Parser.Lex(); // Eat '%'.
        return parseRelocExpr(Operands, PercentLoc);
      }
    }

    MCRegister RegNo;
    SMLoc StartLoc, EndLoc;
    if (parseRegister(RegNo, StartLoc, EndLoc, /*RestoreOnFailure=*/false))
      return true;
    Operands.push_back(M88kOperand::createReg(RegNo, StartLoc, EndLoc));
    return false;
  }

  if (GNUMode && Lexer.is(AsmToken::Identifier)) {
    StringRef Id = Lexer.getTok().getIdentifier();
    SMLoc IdLoc = Parser.getTok().getLoc();

    // Bare hi16(sym) / lo16(sym).
    if ((Id == "hi16" || Id == "lo16") &&
        Lexer.peekTok().is(AsmToken::LParen))
      return parseRelocExpr(Operands, IdLoc);

    // GNU condition names as the first operand of bb0/bb1.
    if (Operands.size() == 1 &&
        (Mnemonic.starts_with("bb0") || Mnemonic.starts_with("bb1"))) {
      int CC = getGNUConditionName(Id);
      if (CC >= 0) {
        SMLoc EndLoc = Lexer.getTok().getEndLoc();
        Parser.Lex();
        Operands.push_back(M88kOperand::createImm(
            MCConstantExpr::create(CC, getContext()), IdLoc, EndLoc));
        return false;
      }
    }

    // Bare register name (r2, x4, cr1, fcr62, ...).
    MCRegister RegNo = matchRegisterName(Id);
    if (RegNo != 0 && Lexer.peekTok().isNot(AsmToken::LParen)) {
      Parser.Lex();
      Operands.push_back(
          M88kOperand::createReg(RegNo, IdLoc, Lexer.getTok().getLoc()));
      return false;
    }
  }

  // Otherwise it is an immediate or address expression: an integer literal,
  // a symbol reference, or a more complex constant expression.
  if (Lexer.is(AsmToken::Integer) || Lexer.is(AsmToken::Identifier) ||
      Lexer.is(AsmToken::Dot) || Lexer.is(AsmToken::Minus) ||
      Lexer.is(AsmToken::Tilde) || Lexer.is(AsmToken::LParen)) {
    SMLoc StartLoc = Parser.getTok().getLoc();
    const MCExpr *Expr;
    if (Parser.parseExpression(Expr))
      return true;
    SMLoc EndLoc = Parser.getTok().getLoc();
    Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));
    return false;
  }

  // Failure.
  return true;
}

bool M88kAsmParser::parseRelocExpr(OperandVector &Operands, SMLoc PercentLoc) {
  // On entry the lexer is on the "hi16"/"lo16" identifier.
  StringRef Id = Parser.getTok().getIdentifier();
  M88kMCExpr::VariantKind Kind = StringSwitch<M88kMCExpr::VariantKind>(Id)
                                     .Case("hi16", M88kMCExpr::VK_ABS_HI)
                                     .Case("lo16", M88kMCExpr::VK_ABS_LO)
                                     .Default(M88kMCExpr::VK_None);
  if (Kind == M88kMCExpr::VK_None)
    return Error(PercentLoc, "unknown relocation specifier");
  Parser.Lex(); // Eat the identifier.

  if (Lexer.isNot(AsmToken::LParen))
    return Error(Parser.getTok().getLoc(), "expected '(' after relocation "
                                           "specifier");
  Parser.Lex(); // Eat '('.

  const MCExpr *SubExpr;
  if (Parser.parseExpression(SubExpr))
    return true;

  if (Lexer.isNot(AsmToken::RParen))
    return Error(Parser.getTok().getLoc(), "expected ')'");
  SMLoc EndLoc = Parser.getTok().getLoc();
  Parser.Lex(); // Eat ')'.

  const MCExpr *Expr = M88kMCExpr::create(Kind, SubExpr, getContext());
  Operands.push_back(M88kOperand::createImm(Expr, PercentLoc, EndLoc));
  return false;
}

ParseStatus M88kAsmParser::parseBFWidth(OperandVector &Operands) {
  // Parses the width of a bitfield. If empty and followed by <O>, then it is 0.
  // If not followed by <O>, then it is the offset, and the width is 0.
  MCContext &Ctx = getContext();
  SMLoc StartLoc = Parser.getTok().getLoc();
  bool HasWidth = false;
  int64_t Width = 0;
  bool IsReallyOffset = false;
  if (Lexer.is(AsmToken::Integer)) {
    Width = Parser.getTok().getIntVal();
    HasWidth = true;
    Parser.Lex();
  }
  if (Lexer.isNot(AsmToken::Less)) {
    if (!HasWidth)
      return ParseStatus::NoMatch;
    IsReallyOffset = true;
  }

  const MCExpr *Expr = MCConstantExpr::create(Width, Ctx);
  SMLoc EndLoc =
      SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  if (IsReallyOffset) {
    Operands.push_back(M88kOperand::createImm(MCConstantExpr::create(0, Ctx),
                                              StartLoc, EndLoc));
    Operands.push_back(M88kOperand::createToken("<", Parser.getTok().getLoc()));
    Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));
    Operands.push_back(M88kOperand::createToken(">", Parser.getTok().getLoc()));
  } else
    Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));

  // Announce match.
  return ParseStatus::Success;
}

ParseStatus M88kAsmParser::parseBFOffset(OperandVector &Operands) {
  // Parses operands like <7>.
  MCContext &Ctx = getContext();
  SMLoc StartLoc = Parser.getTok().getLoc();

  Parser.Lex();
  int64_t Offset;
  if (Lexer.is(AsmToken::Integer))
    Offset = Parser.getTok().getIntVal();
  else if (GNUMode && Lexer.is(AsmToken::Identifier) &&
           getGNUConditionName(Parser.getTok().getIdentifier()) >= 0)
    Offset = getGNUConditionName(Parser.getTok().getIdentifier());
  else
    return ParseStatus::Failure;
  Parser.Lex();
  if (Lexer.isNot(AsmToken::Greater))
    return ParseStatus::Failure;
  Parser.Lex();

  const MCExpr *Expr = MCConstantExpr::create(Offset, Ctx);
  SMLoc EndLoc =
      SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));
  Operands.push_back(M88kOperand::createToken(">", Parser.getTok().getLoc()));

  // Announce match.
  return ParseStatus::Success;
}

ParseStatus M88kAsmParser::parsePixelRot(OperandVector &Operands) {
  // Parses operands like <7>.
  MCContext &Ctx = getContext();
  SMLoc StartLoc = Parser.getTok().getLoc();

  if (Lexer.isNot(AsmToken::Less)) {
    return ParseStatus::NoMatch;
  }
  Parser.Lex();
  if (Lexer.isNot(AsmToken::Integer))
    return ParseStatus::Failure;
  int64_t RotateSize = Parser.getTok().getIntVal();
  Parser.Lex();
  if (Lexer.isNot(AsmToken::Greater))
    return ParseStatus::Failure;
  Parser.Lex();

  if (RotateSize & 0x3) {
    Warning(StartLoc, "Removed lower 2 bits of expression");
    RotateSize &= ~0x3;
  }
  const MCExpr *Expr = MCConstantExpr::create(RotateSize, Ctx);
  SMLoc EndLoc =
      SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));

  // Announce match.
  return ParseStatus::Success;
}

ParseStatus M88kAsmParser::parseConditionCode(OperandVector &Operands) {
  // Parses condition codes for brcond/tcond.
  SMLoc StartLoc = getLexer().getLoc();
  unsigned CC;
  if (Lexer.is(AsmToken::Integer)) {
    int64_t CCVal = Lexer.getTok().getIntVal();
    if (isUInt<5>(CCVal))
      return ParseStatus::NoMatch;
    CC = static_cast<unsigned>(CCVal);
  } else {
    CC = StringSwitch<unsigned>(Parser.getTok().getString())
             .Case("eq0", 0x2)
             .Case("ne0", 0xd)
             .Case("gt0", 0x1)
             .Case("lt0", 0xc)
             .Case("ge0", 0x3)
             .Case("le0", 0xe)
             .Default(0);
    if (CC == 0)
      return ParseStatus::NoMatch;
  }
  Parser.Lex();

  // Create expression.
  SMLoc EndLoc =
      SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  const MCExpr *CCExpr = MCConstantExpr::create(CC, getContext());
  Operands.push_back(M88kOperand::createImm(CCExpr, StartLoc, EndLoc));

  return ParseStatus::Success;
}

ParseStatus M88kAsmParser::parsePCRel(OperandVector &Operands, unsigned Bits) {
  const MCExpr *Expr;
  SMLoc StartLoc = Parser.getTok().getLoc();
  if (getParser().parseExpression(Expr))
    return ParseStatus::NoMatch;

  const int64_t MinVal = -(1LL << Bits);
  const int64_t MaxVal = (1LL << Bits) - 1;
  auto IsOutOfRangeConstant = [&](const MCExpr *E) -> bool {
    if (auto *CE = dyn_cast<MCConstantExpr>(E)) {
      int64_t Value = CE->getValue();
      if ((Value & 1) || Value < MinVal || Value > MaxVal)
        return true;
    }
    return false;
  };

  // For consistency with the GNU assembler, treat immediates as absolute
  // values. In this case, check only the range.
  if (auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    if (IsOutOfRangeConstant(CE)) {
      Error(StartLoc, "offset out of range");
      return ParseStatus::Failure;
    }
  }

  // For consistency with the GNU assembler, conservatively assume that a
  // constant offset must by itself be within the given size range.
  if (const auto *BE = dyn_cast<MCBinaryExpr>(Expr))
    if (IsOutOfRangeConstant(BE->getLHS()) ||
        IsOutOfRangeConstant(BE->getRHS())) {
      Error(StartLoc, "offset out of range");
      return ParseStatus::Failure;
    }

  SMLoc EndLoc =
      SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);

  Operands.push_back(M88kOperand::createImm(Expr, StartLoc, EndLoc));

  return ParseStatus::Success;
}

// Parses register of form %(r|x|cr|fcr)<No>.
bool M88kAsmParser::parseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc, bool RestoreOnFailure) {
  StartLoc = Parser.getTok().getLoc();

  // Eat the '%' prefix.  In GNU mode the prefix is optional.
  bool HavePercent = Parser.getTok().is(AsmToken::Percent);
  if (!HavePercent && !GNUMode)
    return true;
  const AsmToken PercentTok = Parser.getTok();
  if (HavePercent)
    Parser.Lex();

  // Match the register.
  if (Lexer.getKind() != AsmToken::Identifier ||
      (RegNo = matchRegisterName(Lexer.getTok().getIdentifier())) == 0) {
    if (RestoreOnFailure) {
      if (HavePercent)
        Lexer.UnLex(PercentTok);
      else
        return true;
    }
    return Error(StartLoc, "invalid register");
  }

  Parser.Lex(); // Eat identifier token.
  EndLoc = Parser.getTok().getLoc();
  return false;
}

bool M88kAsmParser::parseScaledRegister(OperandVector &Operands) {
  SMLoc LBracketLoc = Parser.getTok().getLoc();

  // Eat the '[' bracket.
  if (Lexer.isNot(AsmToken::LBrac))
    return true;
  Parser.Lex();

  MCRegister RegNo;
  SMLoc StartLoc, EndLoc;
  if (parseRegister(RegNo, StartLoc, EndLoc, /*RestoreOnFailure=*/false))
    return true;

  // Eat the ']' bracket.
  if (Lexer.isNot(AsmToken::RBrac))
    return true;

  SMLoc RBracLoc = Parser.getTok().getLoc();
  Parser.Lex();

  Operands.push_back(M88kOperand::createToken("[", LBracketLoc));
  Operands.push_back(M88kOperand::createReg(RegNo, StartLoc, EndLoc));
  Operands.push_back(M88kOperand::createToken("]", RBracLoc));

  return false;
}

bool M88kAsmParser::parseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {
  return parseRegister(RegNo, StartLoc, EndLoc,
                       /*RestoreOnFailure=*/false);
}

ParseStatus M88kAsmParser::tryParseRegister(MCRegister &RegNo, SMLoc &StartLoc,
                                            SMLoc &EndLoc) {
  bool Result = parseRegister(RegNo, StartLoc, EndLoc,
                              /*RestoreOnFailure=*/true);
  bool PendingErrors = getParser().hasPendingError();
  getParser().clearPendingErrors();
  if (PendingErrors)
    return ParseStatus::Failure;
  if (Result)
    return ParseStatus::NoMatch;
  return ParseStatus::Success;
}

bool M88kAsmParser::matchAndEmitInstruction(SMLoc IdLoc, unsigned &Opcode,
                                            OperandVector &Operands,
                                            MCStreamer &Out,
                                            uint64_t &ErrorInfo,
                                            bool MatchingInlineAsm) {
  // An undotted directive was already emitted by parseGNUDirective.
  if (Operands.size() == 1 &&
      static_cast<M88kOperand &>(*Operands[0]).isToken() &&
      static_cast<M88kOperand &>(*Operands[0]).getToken() ==
          GNUDirectiveSentinel)
    return false;

  MCInst Inst;
  FeatureBitset MissingFeatures;
  unsigned MatchResult = MatchInstructionImpl(
      Operands, Inst, ErrorInfo, MissingFeatures, MatchingInlineAsm);

  switch (MatchResult) {
  case Match_Success:
    Inst.setLoc(IdLoc);
    Out.emitInstruction(Inst, getSTI());
    return false;
  case Match_MissingFeature: {
    assert(MissingFeatures.any() && "Unknown missing features!");
    bool FirstFeature = true;
    std::string Msg = "instruction requires the following:";
    for (unsigned I = 0, E = MissingFeatures.size(); I != E; ++I) {
      if (MissingFeatures[I]) {
        Msg += FirstFeature ? " " : ", ";
        Msg += getSubtargetFeatureName(I);
        FirstFeature = false;
      }
    }
    return Error(IdLoc, Msg);
  }
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IdLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IdLoc, "Too few operands for instruction");
      ErrorLoc = ((M88kOperand &)*Operands[ErrorInfo]).getStartLoc();
    }
    return Error(ErrorLoc, "Invalid operand for instruction");
  }
  case Match_MnemonicFail: {
    FeatureBitset FBS = ComputeAvailableFeatures(getSTI().getFeatureBits());
    M88kOperand &Op = static_cast<M88kOperand &>(*Operands[0]);
    std::string Suggestion = M88kMnemonicSpellCheck(Op.getToken(), FBS, 0);
    return Error(IdLoc, "invalid instruction" + Suggestion/*,
                 Op.getLocRange()*/);
  }
  }

  // Handle the case when the error message is of specific type other than the
  // generic Match_InvalidOperand, and the corresponding operand is missing.
  if (MatchResult > FIRST_TARGET_MATCH_RESULT_TY) {
    SMLoc ErrorLoc = IdLoc;
    if (ErrorInfo != ~0ULL && ErrorInfo >= Operands.size())
      return Error(ErrorLoc, "too few operands for instruction");
  }

  switch (MatchResult) {
  case Match_InvalidBitfieldWidth:
  case Match_InvalidBitfieldOffset:
  case Match_InvalidPixelRotationSize: {
    SMLoc ErrorLoc = ((M88kOperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, getMatchKindDiag((M88kMatchResultTy)MatchResult));
  }
  }

  llvm_unreachable("Unexpected match type");
}

// NOLINTNEXTLINE(readability-identifier-naming)
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeM88kAsmParser() {
  RegisterMCAsmParser<M88kAsmParser> X(getTheM88kTarget());
}
