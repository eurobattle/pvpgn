# Stub used only while locating Drogon, see src/bnetd/CMakeLists.txt.
# Drogon's config requires this package to be present, but Drogon::Drogon does
# not link against it (libdrogon.so pulls in the runtime library on its own).
set(pg_FOUND TRUE)
