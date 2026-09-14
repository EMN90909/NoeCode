#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { mkdirSync, rmSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const compiler = process.argv[2] ? resolve(process.argv[2]) : join(root, "build", process.platform === "win32" ? "noqeri.exe" : "noqeri");
const integration = join(root, "tests", "integration");
const outDir = join(root, "build", "real-programs");
mkdirSync(outDir, { recursive: true });

function invoke(args, options = {}) {
  const result = spawnSync(compiler, args, {
    cwd: root,
    encoding: "utf8",
    env: process.env,
  });
  const expectedStatus = options.status ?? 0;
  if (result.error) throw result.error;
  if (result.status !== expectedStatus) {
    throw new Error([
      `command failed: ${compiler} ${args.join(" ")}`,
      `expected status: ${expectedStatus}`,
      `actual status: ${result.status}`,
      `stdout:\n${result.stdout}`,
      `stderr:\n${result.stderr}`,
    ].join("\n"));
  }
  return { stdout: result.stdout.replace(/\r\n/g, "\n").trim(), stderr: result.stderr };
}

function expectOutput(file, expected) {
  invoke(["check", file]);
  const result = invoke(["run", file]);
  if (result.stdout !== expected) {
    throw new Error(`${file} output mismatch: expected ${JSON.stringify(expected)}, got ${JSON.stringify(result.stdout)}`);
  }
}

console.log("[real-programs] database library service");
expectOutput(join(integration, "database_service.nqr"), "database-service: ok");

console.log("[real-programs] persistent NoqeriDB CRUD");
const dbPath = join(outDir, "users.nqdb");
rmSync(dbPath, { force: true });
const crud = invoke(["db", join(integration, "database_crud.nqd"), dbPath]);
const expectedCrud = "id\tname\tactive\n1\tAda Lovelace\ttrue";
if (crud.stdout !== expectedCrud) throw new Error(`CRUD output mismatch: ${JSON.stringify(crud.stdout)}`);
const readback = invoke(["db", join(integration, "database_readback.nqd"), dbPath]);
const expectedReadback = "id\tname\tactive\n2\tLinus\tfalse";
if (readback.stdout !== expectedReadback) throw new Error(`database persistence mismatch: ${JSON.stringify(readback.stdout)}`);
const duplicate = spawnSync(compiler, ["db", join(integration, "database_duplicate.nqd"), dbPath], { cwd: root, encoding: "utf8" });
if (duplicate.error) throw duplicate.error;
if (duplicate.status === 0) throw new Error("NoqeriDB accepted a duplicate primary key");

console.log("[real-programs] JSON/token service");
expectOutput(join(integration, "json_service.nqr"), "json-service: ok");

console.log("[real-programs] scheduler/concurrency policy");
expectOutput(join(integration, "concurrency_service.nqr"), "concurrency-service: ok");

console.log("[real-programs] host-facing backend/storage APIs compile and lower");
for (const file of ["backend_host_api.nqr", "storage_host_api.nqr"]) {
  const source = join(integration, file);
  invoke(["check", source]);
  invoke(["nir", source]);
}

console.log("[real-programs] web backend emits executable static ESM");
const webSource = join(integration, "backend_web.nqr");
const webOutput = join(outDir, "backend_web.mjs");
invoke(["check", webSource]);
invoke(["web", webSource, webOutput]);
const syntax = spawnSync(process.execPath, ["--check", webOutput], { cwd: root, encoding: "utf8" });
if (syntax.status !== 0) throw new Error(`generated web module failed node --check:\n${syntax.stderr}`);
const generated = await import(`${pathToFileURL(webOutput).href}?v=${Date.now()}`);
const payload = JSON.parse(generated.healthPayload());
if (payload.status !== "ok" || payload.service !== "noqeri") throw new Error("generated health payload is wrong");
if (generated.routeStatus("GET", "/health") !== 200) throw new Error("GET /health routing failed");
if (generated.routeStatus("POST", "/users") !== 201) throw new Error("POST /users routing failed");
if (generated.routeStatus("DELETE", "/users/1") !== 204) throw new Error("DELETE /users/1 routing failed");
if (generated.routeStatus("GET", "/missing") !== 404) throw new Error("404 routing failed");
if (!generated.isSuccess(204) || generated.isSuccess(500)) throw new Error("HTTP success classification failed");
if (generated.responseHasBody(204) || generated.responseHasBody(304) || !generated.responseHasBody(200)) throw new Error("HTTP body semantics failed");

console.log("real-program integration suite: OK");
