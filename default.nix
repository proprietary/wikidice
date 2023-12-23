{ lib
, llvmPackages_17
, cmake
, spdlog
, abseil-cpp }:

llvmPackages_17.stdenv.mkDerivation rec {
  pname = "wikidice";
  version = "0.2.0";
  
  src = ./.;

  nativeBuildInputs = [ cmake ];
  buildInputs = [ spdlog abseil-cpp ];

  cmakeFlags = [
    "-DENABLE_TESTING=OFF"
    "-DENABLE_INSTALL=ON"
  ];
}
