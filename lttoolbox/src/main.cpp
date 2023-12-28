#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/iostream.h>

namespace py = pybind11;
using namespace pybind11::literals;

#include <lttoolbox/acx.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/input_file.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/transducer.h>

class TextOutput {
public:
	UFILE* fp;
	TextOutput(std::string& fn, bool append) {
		fp = u_fopen(fn.c_str(), append ? "ab" : "wb", NULL, NULL);
		// TODO: error handling
	}
	~TextOutput() {
		u_fflush(fp);
		u_fclose(fp);
	}
};

class InputArchive {
public:
	FILE* fp;
	InputArchive(std::string& filename) {
		fp = fopen(filename.c_str(), "rb");
	}
	~InputArchive() {
		fclose(fp);
	}
};

class OutputArchive {
public:
	FILE* fp;
	OutputArchive(std::string& filename) {
		fp = fopen(filename.c_str(), "wb");
	}
	~OutputArchive() {
		fclose(fp);
	}
};

// class for backwards compatibility with old SWIG bindings
class FST: public FSTProcessor {
public:
	/**
	 * Imitates functionality of lt-proc using file path
	 */
	FST(char *dictionary_path) {
		FILE *dictionary = fopen(dictionary_path, "rb");
		load(dictionary);
		fclose(dictionary);
	}

	void lt_proc(int argc, char **argv,
				 const char *input_path, const char *output_path) {
		InputFile input;
		input.open(input_path);
		UFILE* output = u_fopen(output_path, "w", NULL, NULL);
		int cmd = 0;
		int c = 0;
		optind = 1;
		GenerationMode bilmode = gm_unknown;
		while (true) {
			c = getopt(argc, argv, "abcdeglmnoptwxzCIW");
			if (c == -1) {
				break;
			}
			switch(c) {
			case 'a':
			case 'b':
			case 'e':
			case 'g':
			case 'p':
			case 't':
			case 'x':
				if(cmd == 0) {
					cmd = c;
				} else if (cmd == 'g' && c == 'b') {
					cmd = c;
				}
				break;
			case 'c':
				setCaseSensitiveMode(true);
				break;
			case 'd':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_all;
				break;
			case 'l':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_tagged;
				break;
			case 'm':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_tagged_nm;
				break;
			case 'n':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_clean;
				break;
			case 'o':
				if (cmd == 0) cmd = 'b';
				setBiltransSurfaceForms(true);
				break;
			case 'w':
				setDictionaryCaseMode(true);
				break;
			case 'z':
				setNullFlush(true);
				break;
			case 'C':
				if (cmd == 0) cmd = 'g';
				bilmode = gm_carefulcase;
				break;
			case 'I':
				setUseDefaultIgnoredChars(false);
				break;
			case 'W':
				setDisplayWeightsMode(true);
				break;
			default:
				break;
			}
		}

		switch(cmd) {
		case 'b':
			initBiltrans();
			bilingual(input, output, bilmode);
			break;

		case 'e':
			initDecomposition();
			analysis(input, output);
			break;

		case 'g':
			initGeneration();
			generation(input, output, bilmode);
			break;

		case 'p':
			initPostgeneration();
			postgeneration(input, output);
			break;

		case 't':
			initPostgeneration();
			transliteration(input, output);
			break;

		case 'x':
			initPostgeneration();
			intergeneration(input, output);
			break;

		case 'a':
		default:
			initAnalysis();
			analysis(input, output);
			break;
		}

		u_fclose(output);
	}

	void lt_proc_vec(const std::vector<std::string>& args,
					 const std::string& input_path,
					 const std::string& output_path) {
		size_t argc = args.size();
		char** argv = new char*[argc+1];
		for (size_t i = 0; i < argc; i++) {
			argv[i] = new char[args[i].size()+1];
			for (size_t j = 0; j < args[i].size(); j++) {
				argv[i][j] = args[i][j];
			}
			argv[i][args[i].size()] = 0;
		}
		argv[argc] = 0;
		lt_proc(argc, argv,	input_path.c_str(), output_path.c_str());
	}
	void lt_proc_len_vec(int argc, // which we ignore
						 const std::vector<std::string>& args,
						 const std::string& input_path,
						 const std::string& output_path) {
		lt_proc_vec(args, input_path, output_path);
	}
};

PYBIND11_MODULE(lttoolbox, m) {
	m.doc() = "LTTOOLBOX!";

	{
		auto io = m.def_submodule("io", "blah");
		py::class_<InputFile>(io, "InputFile")
			.def(py::init<>())
			.def("open", &InputFile::open)
			.def("open_in_memory", &InputFile::open_in_memory)
			.def("open_or_exit", &InputFile::open_or_exit)
			.def("close", &InputFile::close)
			.def("wrap", &InputFile::wrap)
			.def("get", &InputFile::get)
			.def("peek", &InputFile::peek)
			.def("unget", &InputFile::unget)
			.def("eof", &InputFile::eof)
			.def("rewind", &InputFile::rewind)
			.def("readBlock", &InputFile::readBlock)
			.def("finishWBlank", &InputFile::finishWBlank)
			.def("readBlank", &InputFile::readBlank);

		py::class_<InputArchive>(io, "InputArchive")
			.def(py::init<std::string&>())
			.def("read_u64",
				 [](InputArchive& self) {
					 return read_u64(self.fp);
				 })
			.def("read_u64_le",
				 [](InputArchive& self) {
					 return read_u64_le(self.fp);
				 })
			.def("multibyte_read",
				 [](InputArchive& self) {
					 return Compression::multibyte_read(self.fp);
				 })
			.def("string_read",
				 [](InputArchive& self) {
					 return Compression::string_read(self.fp);
				 })
			.def("long_multibyte_read",
				 [](InputArchive& self) {
					 return Compression::long_multibyte_read(self.fp);
				 });

		py::class_<OutputArchive>(io, "OutputArchive")
			.def(py::init<std::string&>())
			.def("write_u64",
				 [](OutputArchive& self, uint64_t value) {
					 write_u64(self.fp, value);
				 })
			.def("write_u64_le",
				 [](OutputArchive& self, uint64_t value) {
					 write_u64_le(self.fp, value);
				 })
			.def("multibyte_write",
				 [](OutputArchive& self, unsigned int value) {
					 Compression::multibyte_write(value, self.fp);
				 })
			.def("string_write",
				 [](OutputArchive& self, UStringView str) {
					 Compression::string_write(str, self.fp);
				 })
			.def("long_multibyte_write",
				 [](OutputArchive& self, double value) {
					 Compression::long_multibyte_write(value, self.fp);
				 });

		io.attr("HEADER_LTTOOLBOX") = HEADER_LTTOOLBOX;
		py::enum_<LT_FEATURES>(io, "LT_FEATURES")
			.value("LTF_UNKNOWN", LT_FEATURES::LTF_UNKNOWN)
			.value("LTF_RESERVED", LT_FEATURES::LTF_RESERVED)
			.export_values();

		io.attr("HEADER_TRANSDUCER") = HEADER_TRANSDUCER;
		py::enum_<TD_FEATURES>(io, "TD_FEATURES")
			.value("TDF_WEIGHTS", TD_FEATURES::TDF_WEIGHTS)
			.value("TDF_UNKNOWN", TD_FEATURES::TDF_UNKNOWN)
			.value("TDF_RESERVED", TD_FEATURES::TDF_RESERVED)
			.export_values();
	}

	{
		auto fst = m.def_submodule("fst", "blah");

		py::enum_<Alphabet::Side>(fst, "Side")
			.value("Left", Alphabet::Side::left)
			.value("Right", Alphabet::Side::right)
			.export_values();

		py::class_<Alphabet>(fst, "Alphabet")
			.def(py::init<>())
			.def("includeSymbol", &Alphabet::includeSymbol)
			.def("__call__",
				 [](Alphabet& a, UStringView s) {
					 return a(s);
				 })
			.def("__call__",
				 [](Alphabet& a, int32_t c1, int32_t c2) {
					 return a(c1, c2);
				 })
			.def("isSymbolDefined", &Alphabet::isSymbolDefined)
			.def("__len__", &Alphabet::size)
			.def("write",
				 [](Alphabet& self, OutputArchive& output) {
					 self.write(output.fp);
				 })
			.def("read",
				 [](Alphabet& self, InputArchive& input) {
					 self.read(input.fp);
				 })
			.def("writeSymbol",
				 [](Alphabet& a, int32_t symbol, std::string& fn, bool append) {
					 TextOutput o(fn, append);
					 a.writeSymbol(symbol, o.fp);
				 })
			.def("getSymbol",
				 [](Alphabet& a, int32_t symbol) {
					 UString s;
					 a.getSymbol(s, symbol);
					 return s;
				 })
			.def("isTag", &Alphabet::isTag)
			.def("setSymbol", &Alphabet::setSymbol)
			.def("decode", &Alphabet::decode)
			.def("symbolsWhereLeftIs", &Alphabet::symbolsWhereLeftIs)
			.def("createLoopbackSymbols", &Alphabet::createLoopbackSymbols)
			.def("tokenize", &Alphabet::tokenize)
			.def("sameSymbol", &Alphabet::sameSymbol);

		py::class_<Transducer>(fst, "Transducer")
			.def(py::init<>())
			.def("weighted", &Transducer::weighted)
			.def("insertSingleTransduction",
				 &Transducer::insertSingleTransduction)
			.def("insertNewSingleTransduction",
				 &Transducer::insertNewSingleTransduction)
			.def("insertTransducer", &Transducer::insertTransducer)
			.def("linkStates", &Transducer::linkStates)
			.def("isFinal", &Transducer::isFinal)
			.def("recognise", &Transducer::recognise)
			.def("setFinal", &Transducer::setFinal)
			.def("getInitial", &Transducer::getInitial)
			.def("closure",
				 py::overload_cast<int, int>(&Transducer::closure, py::const_))
			.def("closure", py::overload_cast<int, const std::set<int>&>(&Transducer::closure, py::const_))
			.def("closure_all", &Transducer::closure_all)
			.def("joinFinals", &Transducer::joinFinals)
			.def("getFinals", &Transducer::getFinals)
			.def("getTransitions", &Transducer::getTransitions)
			.def("reverse", &Transducer::reverse)
			//.def("show", &Transducer::show) // TODO: need UFILE
			.def("determinize", &Transducer::determinize)
			.def("minimize", &Transducer::minimize)
			.def("optional", &Transducer::optional)
			.def("oneOrMore", &Transducer::oneOrMore)
			.def("zeroOrMore", &Transducer::zeroOrMore)
			.def("clear", &Transducer::clear)
			.def("isEmpty",
				 py::overload_cast<int>(&Transducer::isEmpty, py::const_))
			.def("isEmpty",
				 py::overload_cast<>(&Transducer::isEmpty, py::const_))
			.def("hasNoFinals", &Transducer::hasNoFinals)
			.def("__len__", &Transducer::size)
			.def("numberOfTransitions", &Transducer::numberOfTransitions)
			.def("getStateSize", &Transducer::getStateSize)
			.def("write",
				 [](Transducer& self, OutputArchive& output) {
					 self.write(output.fp);
				 })
			.def("read",
				 [](Transducer& self, InputArchive& input) {
					 self.read(input.fp);
				 })
			.def("unionWith", &Transducer::unionWith)
			.def("appendDotStar", &Transducer::appendDotStar)
			.def("moveLemqsLast", &Transducer::moveLemqsLast)
			.def("copyWithTagsFirst", &Transducer::copyWithTagsFirst)
			.def("trim", &Transducer::trim)
			.def("compose", &Transducer::compose)
			.def("composeLabel", &Transducer::composeLabel)
			.def("updateAlphabet", &Transducer::updateAlphabet)
			.def("invert", &Transducer::invert)
			.def("deleteSymbols", &Transducer::deleteSymbols)
			.def("epsilonizeSymbols", &Transducer::epsilonizeSymbols)
			.def("applyACX", &Transducer::applyACX);

		fst.def("readACX", &readACX);
	}

	{
		// skip functions that are in python stdlib
		auto su = m.def_submodule("string_utils", "blah");
		su.def("split_escaped", &StringUtils::split_escaped, "blah");
		su.def("getcase", &StringUtils::getcase, "blah");
		su.def("copycase", &StringUtils::copycase, "blah");
		su.def("caseequal", &StringUtils::caseequal, "blah");
		su.def("merge_wblanks", &StringUtils::merge_wblanks, "blah");
	}

	{
		auto proc = m.def_submodule("proc", "blah");

		py::enum_<GenerationMode>(proc, "GenerationMode")
			.value("gm_clean", GenerationMode::gm_clean)
			.value("gm_unknown", GenerationMode::gm_unknown)
			.value("gm_all", GenerationMode::gm_all)
			.value("gm_tagged", GenerationMode::gm_tagged)
			.value("gm_tagged_nm", GenerationMode::gm_tagged_nm)
			.value("gm_carefulcase", GenerationMode::gm_carefulcase)
			.export_values();

		py::class_<FSTProcessor>(proc, "FSTProcessor")
			.def(py::init<>())
			.def("initAnalysis", &FSTProcessor::initAnalysis)
			.def("initTMAnalysis", &FSTProcessor::initTMAnalysis)
			.def("initSAO", &FSTProcessor::initSAO)
			.def("initGeneration", &FSTProcessor::initGeneration)
			.def("initPostgeneration", &FSTProcessor::initPostgeneration)
			.def("initTransliteration", &FSTProcessor::initTransliteration)
			.def("initBiltrans", &FSTProcessor::initBiltrans)
			.def("initDecomposition", &FSTProcessor::initDecomposition)
			/*.def("analysis", &FSTProcessor::analysis)
			.def("tm_analysis", &FSTProcessor::tm_analysis)
			.def("generation", &FSTProcessor::generation)
			.def("postgeneration", &FSTProcessor::postgeneration)
			.def("intergeneration", &FSTProcessor::intergeneration)
			.def("transliteration", &FSTProcessor::transliteration)*/
			.def("biltrans", &FSTProcessor::biltrans)
			.def("biltransfull", &FSTProcessor::biltransfull)
			.def("bilingual",
				 [](FSTProcessor& self, InputFile& input, std::string& fname,
					GenerationMode mode, bool append) {
					 TextOutput o(fname, append);
					 self.bilingual(input, o.fp, mode);
				 }, "input"_a, "fname"_a, "mode"_a=gm_unknown, "append"_a=false)
			.def("biltransWithQueue", &FSTProcessor::biltransWithQueue)
			.def("biltransWithoutQueue", &FSTProcessor::biltransWithoutQueue)
			//.def("SAO", &FSTProcessor::SAO)
			.def("parseICX", &FSTProcessor::parseICX)
			.def("parseRCX", &FSTProcessor::parseRCX)
			.def("load",
				 [](FSTProcessor& self, InputArchive& input) {
					 self.load(input.fp);
				 })
			.def("valid", &FSTProcessor::valid)
			.def("setCaseSensitiveMode", &FSTProcessor::setCaseSensitiveMode)
			.def("setDictionaryCaseMode", &FSTProcessor::setDictionaryCaseMode)
			.def("setBiltransSurfaceForms", &FSTProcessor::setBiltransSurfaceForms)
			.def("setIgnoredChars", &FSTProcessor::setIgnoredChars)
			.def("setRestoreChars", &FSTProcessor::setRestoreChars)
			.def_property("null_flush", &FSTProcessor::getNullFlush, &FSTProcessor::setNullFlush)
			.def("setUseDefaultIgnoredChars", &FSTProcessor::setUseDefaultIgnoredChars)
			.def("setDisplayWeightsMode", &FSTProcessor::setDisplayWeightsMode)
			.def("setMaxAnalysesValue", &FSTProcessor::setMaxAnalysesValue)
			.def("setMaxWeightClassesValue", &FSTProcessor::setMaxWeightClassesValue)
			.def("setCompoundMaxElements", &FSTProcessor::setCompoundMaxElements)
			.def("getDecompoundingMode", &FSTProcessor::getDecompoundingMode);
	}

	py::class_<FST>(m, "FST")
		.def(py::init<char*>())
		.def("lt_proc", &FST::lt_proc_vec)
		.def("lt_proc", &FST::lt_proc_len_vec);
}
