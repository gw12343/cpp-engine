#include "EnginePaths.h"

#include "core/ProjectSettings.h"

#include <filesystem>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Engine {
	namespace fs = std::filesystem;

	namespace {
		bool StartsWithFolder(const std::string& generic, const char* folder)
		{
			const std::string prefix = std::string(folder) + "/";
			return generic == folder || generic.rfind(prefix, 0) == 0;
		}

		std::string Generic(const fs::path& p)
		{
			return p.generic_string();
		}

		bool IsUnder(const fs::path& path, const fs::path& root, fs::path& relativeOut)
		{
			std::error_code ec;
			if (root.empty()) {
				return false;
			}
			const fs::path rel = fs::relative(path, root, ec);
			if (ec || rel.empty()) {
				return false;
			}
			const std::string s = rel.generic_string();
			if (s == ".." || s.rfind("../", 0) == 0) {
				return false;
			}
			relativeOut = rel;
			return true;
		}

		fs::path ExeDirectory()
		{
#ifdef _WIN32
			char buf[MAX_PATH] = {};
			const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
			if (n > 0 && n < MAX_PATH) {
				return fs::path(buf).parent_path();
			}
#endif
			return {};
		}
	} // namespace

	EnginePaths& EnginePaths::Get()
	{
		static EnginePaths instance;
		if (instance.m_engineRoot.empty()) {
			instance.Detect();
		}
		return instance;
	}

	void EnginePaths::Detect()
	{
		std::error_code ec;
		const fs::path  cwd = fs::current_path(ec);
		const fs::path  exe = ExeDirectory();

		if (!cwd.empty() && fs::exists(cwd / "resources", ec)) {
			m_engineRoot = Generic(fs::weakly_canonical(cwd, ec));
			return;
		}
		if (!exe.empty() && fs::exists(exe / "resources", ec)) {
			m_engineRoot = Generic(fs::weakly_canonical(exe, ec));
			return;
		}
		if (!cwd.empty()) {
			m_engineRoot = Generic(fs::weakly_canonical(cwd, ec));
			if (m_engineRoot.empty()) {
				m_engineRoot = Generic(cwd);
			}
			return;
		}
		m_engineRoot = Generic(exe);
	}

	std::string EnginePaths::ResourcesDir() const
	{
		return Generic(fs::path(m_engineRoot) / "resources");
	}

	std::string EnginePaths::ExecutableDirectory() const
	{
		return Generic(ExeDirectory());
	}

	std::string EnginePaths::Resolve(const std::string& path) const
	{
		if (path.empty()) {
			return {};
		}
		fs::path p(path);
		if (p.is_absolute()) {
			return Generic(p);
		}

		const std::string generic = p.generic_string();
		if (StartsWithFolder(generic, "resources")) {
			return Generic(fs::path(m_engineRoot) / p);
		}

		if (GetProject().IsOpen()) {
			const fs::path root(GetProject().Root());
			if (StartsWithFolder(generic, "assets") || StartsWithFolder(generic, "scenes") ||
			    StartsWithFolder(generic, "scripts")) {
				return Generic(root / p);
			}
			const fs::path inProject = root / p;
			std::error_code ec;
			if (fs::exists(inProject, ec)) {
				return Generic(inProject);
			}
		}

		const fs::path inEngine = fs::path(m_engineRoot) / p;
		std::error_code ec;
		if (fs::exists(inEngine, ec)) {
			return Generic(inEngine);
		}
		if (GetProject().IsOpen()) {
			return Generic(fs::path(GetProject().Root()) / p);
		}
		return Generic(inEngine);
	}

	std::string EnginePaths::ToLogical(const std::string& path) const
	{
		if (path.empty()) {
			return {};
		}
		std::error_code ec;
		fs::path        abs = fs::absolute(path, ec);
		if (ec) {
			return fs::path(path).generic_string();
		}
		abs = fs::weakly_canonical(abs, ec);

		fs::path rel;
		if (GetProject().IsOpen() && IsUnder(abs, fs::path(GetProject().Root()), rel)) {
			return Generic(rel);
		}
		if (IsUnder(abs, fs::path(m_engineRoot), rel)) {
			return Generic(rel);
		}
		return fs::path(path).generic_string();
	}

	bool EnginePaths::Exists(const std::string& path) const
	{
		if (path.empty()) {
			return false;
		}
		std::error_code ec;
		return fs::exists(Resolve(path), ec);
	}

} // namespace Engine
