/*********************************************************************************************/
/* VITESS module to compare events to a reference file                                       */
/*                                                                                           */
/* To be used in module tests.                                                               */
/*                                                                                           */
/*      Apr 2026  F. Beule        initial version                                            */
/*********************************************************************************************/

#include "init.h"
#include "softabort.h"
#include "statistics.h"
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif


class CompareModule
{
public:
  int main(int argc, char **argv);

  McCompID moduleId() { return MCN_COMPARE; }

  char const *moduleVersion() { return "1.0"; }

  bool parseArg(char prefix, char const *arg);

  bool init();

  void processNeutron(Neutron *neutron);

  void cleanup();

  bool compareEvent(Neutron const *neutron, Neutron const *ref);

  bool compareDouble(double value, double ref) const;

  void printError(char const *format, ...) const;

  static double parseDouble(char const *arg);

  static bool readVitessEvent(Neutron *neutron, FILE *f);

  static void formatVitessEvent(Neutron const *neutron, char *buf, size_t size);

  static int compareId(Neutron const *a, Neutron const *b);

  static void printStats(FILE *stream, Statistics::Result const &ref, Statistics::Result const &act);

private:
  char const *inputFileName_ = nullptr; /// -A  input file name
  double err_rel_ = 0.0;                /// -r  maximum relative error
  double err_abs_ = 0.0;                /// -a  maximum absolute error

  FILE *inputFilePtr_ = nullptr;
  bool ok_ = true;     /// no error or deviation from reference detected
  bool active_ = true; /// no error/EOF so far
  Neutron reference_;
  bool has_reference_ = false;
  Statistics stats_ref_{};
  Statistics stats_act_{};
  struct
  {
    int total = 0;
    int missing = 0;
    int excess = 0;
    int trace = 0;
    int time = 0;
    int wavelength = 0;
    int intensity = 0;
    int position = 0;
    int direction = 0;
    int spin = 0;
  } counter_;

  static constexpr int MAX_ERROR_COUNT = 10;
};

int main(int argc, char **argv)
{
  CompareModule mod;
  return mod.main(argc, argv);
}

int CompareModule::main(int argc, char **argv)
{
  _eModule = moduleId();
  Init(argc, argv, _eModule);
  PrintModuleName(_eModule, moduleVersion());

  for (int i = 1; i < argc; i++) {
    char const *arg = argv[i];
    /* Error when argument is to short or does not start with a dash and a letter
     * ignore double dashes (global parameters) and processed parameters (starting with '+') */
    if (strlen(arg) < 3 || (arg[0] != '-' && arg[0] != '+') || (!isalpha(arg[1]) && arg[1] != '-')) {
      Error2("Invalid command line argument", arg);
    }
    if (arg[0] == '+' || arg[1] == '-') {
      continue;
    }
    if (!parseArg(arg[1], &arg[2])) {
      Error2("Invalid command line argument", arg);
    }
  }
  if (!init()) {
    Error("Failed to intialize module");
  }

  DECLARE_ABORT;
  while (ReadNeutrons() != 0) {
    for (int i = 0; i < NumNeutGot; i++) {
      CHECK
      processNeutron(&InputNeutrons[i]);
    }
  }

my_exit:
  cleanup();
  Cleanup(0.0, 0.0, 0.0, 0.0, 0.0);

  return counter_.total != 0;
}

bool CompareModule::parseArg(char prefix, char const *arg)
{
  switch (prefix) {
  case 'A':
    inputFileName_ = arg;
    break;
  case 'r':
    err_rel_ = parseDouble(arg);
    break;
  case 'a':
    err_abs_ = parseDouble(arg);
    break;
  default:
    return false;
  }
  return true;
}

bool CompareModule::init()
{
  if (inputFileName_ == nullptr) {
    Error("No input file given");
  }
  inputFilePtr_ = OpenInputFile(inputFileName_, true, "r");
  if (inputFilePtr_ == nullptr) {
    Error2("Cannot open input file", inputFileName_);
  }
  return true;
}

void CompareModule::processNeutron(Neutron *neutron)
{
  stats_act_.addNeutron(*neutron);
  if (!active_) {
    WriteNeutron(neutron);
    return;
  }
  // find matching reference neutron
  while (!has_reference_) {
    if (feof(inputFilePtr_)) {
      printError("%s", "End of reference file reached");
      ok_ = false;
      active_ = false;
      break;
    }
    if (!readVitessEvent(&reference_, inputFilePtr_)) {
      printError("%s", "Could not read reference event");
      ok_ = false;
      active_ = false;
      break;
    }
    stats_ref_.addNeutron(reference_);
    if (compareId(&reference_, neutron) < 0) {
      if (counter_.total < MAX_ERROR_COUNT) {
        char s_ref[CHAR_BUF_SMALL];
        formatVitessEvent(&reference_, s_ref, sizeof(s_ref));
        printError("- %s", s_ref);
      }
      counter_.missing++;
      counter_.total++;
      ok_ = false;
    } else {
      has_reference_ = true;
    }
  }
  if (!has_reference_ || compareId(&reference_, neutron) > 0) {
    if (counter_.total < MAX_ERROR_COUNT) {
      char s_act[CHAR_BUF_SMALL];
      formatVitessEvent(neutron, s_act, sizeof(s_act));
      printError("+ %s", s_act);
    }
    counter_.excess++;
    counter_.total++;
    ok_ = false;
  } else {
    has_reference_ = false;
    if (!compareEvent(neutron, &reference_)) {
      if (counter_.total < MAX_ERROR_COUNT) {
        char s_ref[CHAR_BUF_SMALL], s_act[CHAR_BUF_SMALL];
        formatVitessEvent(&reference_, s_ref, sizeof(s_ref));
        formatVitessEvent(neutron, s_act, sizeof(s_act));
        printError("- %s", s_ref);
        printError("+ %s", s_act);
      }
      ok_ = false;
    }
  }
  WriteNeutron(neutron);
}

void CompareModule::cleanup()
{
  fprintf(LogFilePtr,
          "Error counts: total=%d, missing=%d, excess=%d, trace=%d, time=%d, wavelength=%d, intensity=%d, "
          "position=%d, direction=%d, spin=%d\n",
          counter_.total, counter_.missing, counter_.excess, counter_.trace, counter_.time, counter_.wavelength,
          counter_.intensity, counter_.position, counter_.direction, counter_.spin);
  printStats(LogFilePtr, stats_ref_.result(), stats_act_.result());

  if (!ok_) {
    char const *pre = "";
    char const *post = "";
#ifndef WIN32
    if (isatty(fileno(LogFilePtr))) {
      pre = "\033[31m";
      post = "\033[39m";
    }
#endif
    if (counter_.total > 0) {
      fprintf(LogFilePtr, "%sERROR: %d comparisons failed\n%s", pre, counter_.total, post);
    } else {
      fprintf(LogFilePtr, "%sERROR: comparison failed\n%s", pre, post);
    }
  }
  if (inputFilePtr_ != nullptr) {
    fclose(inputFilePtr_);
    inputFilePtr_ = nullptr;
  }
}

void CompareModule::printError(char const *format, ...) const
{
  char fullFormat[CHAR_BUF_SMALL];
  snprintf(fullFormat, sizeof(fullFormat), "COMPARE: %s\n", format);
  va_list args;
  va_start(args, format);
  vfprintf(LogFilePtr, fullFormat, args);
  va_end(args);
  fflush(LogFilePtr);
}

bool CompareModule::compareEvent(Neutron const *neut, Neutron const *ref)
{
  bool a_eob = IsEOB(neut);
  bool b_eob = IsEOB(ref);
  if (a_eob != b_eob) {
    counter_.total++;
    return false;
  }
  if (a_eob) return true;
  bool res = true;
  if (neut->ID.IDNo != ref->ID.IDNo || neut->ID.IDGrp[0] != ref->ID.IDGrp[0] || neut->ID.IDGrp[1] != ref->ID.IDGrp[1] ||
      neut->Debug != ref->Debug || neut->Color != ref->Color) {
    counter_.trace++;
    res = false;
  }
  if (!compareDouble(neut->Time, ref->Time)) {
    counter_.time++;
    res = false;
  }
  if (!compareDouble(neut->Wavelength, ref->Wavelength)) {
    counter_.wavelength++;
    res = false;
  }
  if (!compareDouble(neut->Probability, ref->Probability)) {
    counter_.intensity++;
    res = false;
  }
  if (!compareDouble(neut->Position[0], ref->Position[0]) || !compareDouble(neut->Position[1], ref->Position[1]) ||
      !compareDouble(neut->Position[2], ref->Position[2])) {
    counter_.position++;
    res = false;
  }
  if (!compareDouble(neut->Vector[0], ref->Vector[0]) || !compareDouble(neut->Vector[1], ref->Vector[1]) ||
      !compareDouble(neut->Vector[2], ref->Vector[2])) {
    counter_.direction++;
    res = false;
  }
  if (!compareDouble(neut->Spin[0], ref->Spin[0]) || !compareDouble(neut->Spin[1], ref->Spin[1]) ||
      !compareDouble(neut->Spin[2], ref->Spin[2])) {
    counter_.spin++;
    res = false;
  }
  if (!res) {
    counter_.total++;
  }
  return res;
}

bool CompareModule::compareDouble(double value, double ref) const
{
  double e = fabs(value - ref);
  return e <= err_abs_ || e <= err_rel_ * fabs(ref);
}

double CompareModule::parseDouble(char const *arg)
{
  char *endptr = nullptr;
  double val = strtod(arg, &endptr);
  if (endptr == nullptr || endptr == arg) {
    Error2("Invalid floating-point argument", arg);
  }
  return val;
}

bool CompareModule::readVitessEvent(Neutron *neutron, FILE *f)
{
  char const *format = "%c%c%lu %c %hd %lf %le %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf";
  char line[CHAR_BUF_SMALL];
  int rcs;

  InitNeutron(neutron);

  if (ReadLine(f, line, sizeof(line))) {
    rcs = sscanf(line, format, &neutron->ID.IDGrp[0], &neutron->ID.IDGrp[1], &neutron->ID.IDNo, &neutron->Debug,
                 &neutron->Color, &neutron->Time, &neutron->Wavelength, &neutron->Probability, &neutron->Position[0],
                 &neutron->Position[1], &neutron->Position[2], &neutron->Vector[0], &neutron->Vector[1],
                 &neutron->Vector[2], &neutron->Spin[0], &neutron->Spin[1], &neutron->Spin[2]);
    return rcs > 10;
  }

  return false;
}

void CompareModule::formatVitessEvent(Neutron const *neut, char *buf, size_t size)
{
  char const *fmt = "%c%c%010lu %c %5d %.5e %.5e %.5e % .5e % .5e % .5e % .5e % .5e % .5e % .5e % .5e % .5e";
  snprintf(buf, size, fmt, neut->ID.IDGrp[0], neut->ID.IDGrp[1], neut->ID.IDNo, neut->Debug, neut->Color, neut->Time,
           neut->Wavelength, neut->Probability, neut->Position[0], neut->Position[1], neut->Position[2],
           neut->Vector[0], neut->Vector[1], neut->Vector[2], neut->Spin[0], neut->Spin[1], neut->Spin[2]);
}

int CompareModule::compareId(Neutron const *a, Neutron const *b)
{
  if (a->ID.IDNo > b->ID.IDNo) return 1;
  if (a->ID.IDNo < b->ID.IDNo) return -1;
  if (a->ID.IDGrp[1] > b->ID.IDGrp[1]) return 1;
  if (a->ID.IDGrp[1] < b->ID.IDGrp[1]) return -1;
  if (a->ID.IDGrp[0] > b->ID.IDGrp[0]) return 1;
  if (a->ID.IDGrp[0] < b->ID.IDGrp[0]) return -1;
  return 0;
}

void CompareModule::printStats(FILE *stream, Statistics::Result const &ref, Statistics::Result const &act)
{
  constexpr char const *fmt_str = "%-22s %16s %16s\n";
  constexpr char const *fmt_u64 = "%-22s %16" PRIu64 " %16" PRIu64 "\n";
  constexpr char const *fmt_double = "%-22s %16.12g %16.12g\n";
  fprintf(stream, fmt_str, "", "Reference Beam", "Processed Beam");
  fprintf(stream, fmt_u64, "Events", ref.nevents, act.nevents);
  fprintf(stream, fmt_double, "Total Intensity", ref.intensity, act.intensity);
  fprintf(stream, fmt_double, "Wavelength mean", ref.lambda_mean, act.lambda_mean);
  fprintf(stream, fmt_double, "Wavelength RMS", sqrt(ref.lambda_var), sqrt(act.lambda_var));
  fprintf(stream, fmt_double, "Divergence abs mean Y", ref.divergence[0], act.divergence[0]);
  fprintf(stream, fmt_double, "Divergence abs mean Z", ref.divergence[1], act.divergence[1]);
}
