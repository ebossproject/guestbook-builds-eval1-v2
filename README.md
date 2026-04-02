> ⚠️ **SECURITY & USE NOTICE**
> 
> Unless specified otherwise, the contents of this repository are distributed according to the following terms.
> 
> Copyright (c) 2024-2026 Cromulence LLC. All rights reserved.
> 
> This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001124C0487. Any opinions, finding and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of DARPA.
> 
> The U.S. Government has unlimited rights to this software under contract HR001124C0487.
>
> DISTRIBUTION STATEMENT A. Unlimited Distribution
>
> Original work Licensed under MIT License (See `LICENSE`). Derived from open source libraries under respective licenses - see (`THIRD_PARTY_LICENSES.md`)
>
> This repository is provided **solely for research, educational, and defensive security analysis purposes**.
>
> The software contained herein **intentionally includes known vulnerabilities** in third-party libraries and application logic. These vulnerabilities are included **for the purpose of studying reachability analysis, exploitability, and remediation techniques**.
>
> **DO NOT deploy this software in production environments**, on internet-accessible systems, or on systems containing sensitive data.
>
> By downloading, building, or running this software, you acknowledge that you **understand its intended purpose and potential security impact**, and that you assume all responsibility for its use.


# guestbook-builds-eval1

## Objective - Reachability at Scale
This challenge includes multiple vulnerabilities across 128 unique variants of the Guestbook software. The objective for this challenge is to build, test, and output the reachability analysis classification for each one of the 128 varints. Reachability results must be in the accepted json format.

## Overview
The Guestbook challenge is created as a simple web application that allows guestbook entries to be added, stored, and retrieved from a database. It relies on the **sqlite3**, **crow**, **libsodium**, and **libasio** libraries. For this release, synthetic vulnerabilities have been placed in the **sqlite3** and **crow** libraries. Depending on the configuration of the libraries and the Guestbook application source code, these vulnerabilities may or may not be reachable. Utilizing custom tooling, performers must assess the reachability of each vulnerability in all variants of Guestbook.

## Versions
The 128 variants of Guestbook are created by permutations of 16 versions of the Guestbook application source, 4 versions of the Crow source, and 2 verisons of the sqlite3 source.

The guestbook source versions are aligned with the variant minor release version (major_release.**minor_release**.patch_version).

The dependencies will consitute a change in the **patch_version** of the variant.
- Versions 0.x.0, 0.x.2, 0.x.4, and 0.x.6 contain sqlite 3.47.2
- Versions 0.x.1, 0.x.3, 0.x.5, and 0.x.7 contain sqlite 3.47.99
- Versions 0.x.0 and 0.x.1 contain crow 0.0.1
- Versions 0.x.2 and 0.x.3 contain crow 0.1.0
- Versions 0.x.4 and 0.x.5 contain crow 0.2.0
- Versions 0.x.6 and 0.x.7 contain crow 1.0.0

The `variants.csv` file is included as a reference chart. It does not include any unmodified or static dependencies (libsodium, libasio) that are directly managed by vcpkg without a custom port. 

These version numbers are reflected in the `variant-builds` directory.

## Attack Surface
The Guestbook application responds to commands over a network connection. The attack surface for this challenge includes items sent over the network, and may include commands that involve files local to the machine such as the **mustache** files used to render sites and filter or display information. These files are not attacker controlled, but should be considerd as part of the inputs to the system.

## Vulnerability Details
### CVE-guestbook-001
sqlite3 version **3.47.99** allows an invalid SQL query to trigger a Denial of Service vulnerability in sqlite.c. This vulnerability can be triggered by a SQL command that calls the abort function, including as the payload to a SQL injection.

### CVE-crow-002
crow versions **0.0.1 through 0.1.0 inclusive** supports a `crow-env` mustache tag in templates that can expose sensitive information from environment variables in `mustache.h`. This vulnerability can be triggered by a template that contains a `crow-env` tag, even if the context of the template defines content for that tag. CWE-489: Active Debug Code.

### CVE-crow-003
crow version **0.0.1 and 0.2.0** does not properly handle query parameters. It will continue to process query parameters, overflowing the vector of results. CWE-787: Out-of-bounds Write.

## Usage
To build, use the following commands:

To build the dockerfiles, run the following command `VARIANT=X.X.X docker compose build`OR modify the **VARIANT** field in the `compose.yml` file to reflect the desired variants.

Variant 0.1.1 is vulnerable to all PoVs, so this is a good example to start testing with.

To start the challenge server, run the following command `docker compose up -d guestbook`

To run the poller, run the following command `docker compose up poller`

To run a specific proof of vulnerability, run the following command `docker compose run --rm <vulnerability>`

A list of vulnerabilities can be found within the `pov` folder or under the Vulnerability Details secion

## Build
This updated challenge structure relies on vcpkg package manager and cmake working coopreratively to build stb-resize. This challenge is developed primarily to be built using the Dockerfile locally with Compose. It is viable to build this locally outside the Dockerfile process, however you must have a foundational understanding of vcpkg and cmake, supply your own toolchains and triplets, and make note of the  environmental variables here.

In this form, cmake will create the build directory, chainload the custom toolchain file to install the dependencies, and build for Release according to the target triplet.

## Vulnerability Details
The included `vulnerabilities.json` file contains details about each of the CVEs present in this challenge.

List of available pov tests:
- `pov_info_env`
- `pov_filter_column`
- `pov_many_query_params`
- `pov_sort_column`
- `pov_sort_direction`

In this challenge, certain vulnerabilities may have multiple execution paths (i.e. mutliple PoV attempts may be required to trigger a vulnerability.)

## Configuration

The following 

- `VARIANT` - Build arg set in the `compose.yml` file; specifies which variant to build
- `VCPKG_OVERLAY_TRIPLETS` - specifies directory to look for target triplets
- `VCPKG_OVERLAY_PORTS` - specifies location to search ports (takes precedence over vcpkg repo)
- `TOOLCHAIN_FILE` - specifies toolchain path and file
- `TRIPLET` - specifies the triplet to use for the cmake configuration
- `VCPKG_ROOT` - specifies the root of the vcpkg install location and other contents

## Reachability
This challenge is focused on determining reachability with high accuracy and precision for a large number of instances and vulnerabilities. This simulates diversely configured applications being widely deployed such as in an enterprise setting. 

A table of results is provided in `guestbook_reachability.csv`. 

## Attributions
SQLite 3 source has been modified from the [SQLite](https://sqlite.org/) project available via public domain, read [here.](https://sqlite.org/copyright.html)

Crow source has been modified from the [Crow](https://github.com/CrowCpp/Crow) project available under the BSD-3 clause license available [here.](https://github.com/CrowCpp/Crow/blob/master/LICENSE)