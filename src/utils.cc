// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "utils.hh"

#include "log.hh"
#include "platform.hh"

#include <siphash.h>

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
using namespace llvm;

#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <functional>
#include <string.h>
#include <unordered_map>

namespace ccls {
uint64_t HashUsr(llvm::StringRef s) {
  union {
    uint64_t ret;
    uint8_t out[8];
  };
  // k is an arbitrary key. Don't change it.
  const uint8_t k[16] = {0xd0, 0xe5, 0x4d, 0x61, 0x74, 0x63, 0x68, 0x52,
                         0x61, 0x79, 0xea, 0x70, 0xca, 0x70, 0xf0, 0x0d};
  (void)siphash(reinterpret_cast<const uint8_t *>(s.data()), s.size(), k, out,
                8);
  return ret;
}

std::string LowerPathIfInsensitive(const std::string &path) {
#if defined(_WIN32)
  std::string ret = path;
  for (char &c : ret)
    c = tolower(c);
  return ret;
#else
  return path;
#endif
}

void EnsureEndsInSlash(std::string &path) {
  if (path.empty() || path[path.size() - 1] != '/')
    path += '/';
}

std::string EscapeFileName(std::string path) {
  bool slash = path.size() && path.back() == '/';
  for (char &c : path)
    if (c == '\\' || c == '/' || c == ':')
      c = '@';
  if (slash)
    path += '/';
  return path;
}

std::string ResolveIfRelative(const std::string &directory,
                              const std::string &path) {
  if (sys::path::is_absolute(path))
    return path;
  SmallString<256> Ret;
  sys::path::append(Ret, directory, path);
  return NormalizePath(Ret.str());
}

std::optional<int64_t> LastWriteTime(const std::string &path) {
  sys::fs::file_status Status;
  if (sys::fs::status(path, Status))
    return {};
  return sys::toTimeT(Status.getLastModificationTime());
}

std::optional<std::string> ReadContent(const std::string &filename) {
  char buf[4096];
  std::string ret;
  FILE *f = fopen(filename.c_str(), "rb");
  if (!f)
    return {};
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0)
    ret.append(buf, n);
  fclose(f);
  return ret;
}

void WriteToFile(const std::string &filename, const std::string &content) {
  FILE *f = fopen(filename.c_str(), "wb");
  if (!f ||
      (content.size() && fwrite(content.c_str(), content.size(), 1, f) != 1)) {
    LOG_S(ERROR) << "failed to write to " << filename << ' ' << strerror(errno);
    return;
  }
  fclose(f);
}

// Find discontinous |search| in |content|.
// Return |found| and the count of skipped chars before found.
int ReverseSubseqMatch(std::string_view pat, std::string_view text,
                       int case_sensitivity) {
  if (case_sensitivity == 1)
    case_sensitivity = std::any_of(pat.begin(), pat.end(), isupper) ? 2 : 0;
  int j = pat.size();
  if (!j)
    return text.size();
  for (int i = text.size(); i--;)
    if ((case_sensitivity ? text[i] == pat[j - 1]
                          : tolower(text[i]) == tolower(pat[j - 1])) &&
        !--j)
      return i;
  return -1;
}

std::string GetDefaultResourceDirectory() { return DEFAULT_RESOURCE_DIRECTORY; }
}
