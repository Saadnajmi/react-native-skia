import { spawn, execSync } from "child_process";
import fs, {
  copyFileSync,
  existsSync,
  mkdirSync,
  readdirSync,
  statSync,
} from "fs";
import { exit } from "process";
import path from "path";

export const ensureFolderExists = (dirPath: string) => {
  try {
    console.log(`Ensuring that ${dirPath} exists...`);
    mkdirSync(dirPath, { recursive: true });
  } catch (err) {
    // eslint-disable-next-line @typescript-eslint/ban-ts-comment
    // @ts-ignore
    if (err.code !== "EEXIST") {
      throw err;
    }
  }
};

export const runAsync = (command: string, label: string): Promise<void> => {
  return new Promise((resolve, reject) => {
    const [cmd, ...args] = command.split(" ");
    console.log({ cmd, args });
    // build-skia builds POSIX-style command lines (e.g.
    // `PATH=../depot_tools/:$PATH gn gen ...`). On macOS/Linux,
    // spawn(..., {shell: true}) routes through /bin/sh which parses the
    // `PATH=` prefix correctly. On Windows the same option defaults to
    // cmd.exe, which interprets `PATH=...` as a `set` and never invokes
    // the trailing tool. Force bash on Windows so the existing command
    // lines work unchanged.
    const shell: true | string =
      process.platform === "win32" ? "bash" : true;
    const childProcess = spawn(cmd, args, {
      shell,
    });

    childProcess.stdout.on("data", (data) => {
      process.stdout.write(`${label} ${data}`);
    });

    childProcess.stderr.on("data", (data) => {
      console.error(`${label} ${data}`);
    });

    childProcess.on("close", (code) => {
      if (code === 0) {
        resolve();
      } else {
        reject(new Error(`${label} exited with code ${code}`));
      }
    });

    childProcess.on("error", (error) => {
      reject(new Error(`${label} ${error.message}`));
    });
  });
};

export const mapKeys = <T extends object>(obj: T) =>
  Object.keys(obj) as (keyof T)[];

export const checkFileExists = (filePath: string) => {
  const exists = existsSync(filePath);
  if (!exists) {
    console.log("");
    console.log("Failed:");
    console.log(filePath + " not found. (" + filePath + ")");
    console.log("");
    exit(1);
  } else {
    console.log("✅ " + filePath);
  }
};

export const $ = (command: string) => {
  try {
    // Same Windows-shell consideration as runAsync: route through bash so
    // POSIX command lines (PATH=... cmd, here-strings, etc.) work unchanged.
    return execSync(
      command,
      process.platform === "win32" ? { shell: "bash" } : undefined
    );
  } catch (e) {
    exit(1);
  }
};

const serializeCMakeArgs = (args: Record<string, string>) => {
  return Object.keys(args)
    .map((key) => `-D${key}=${args[key]}`)
    .join(" ");
};

export const build = async (
  label: string,
  args: Record<string, string>,
  debugLabel: string
) => {
  console.log(`🔨 Building ${label}`);
  $(`mkdir -p externals/dawn/out/${label}`);
  process.chdir(`externals/dawn/out/${label}`);
  const cmd = `cmake ../.. -G Ninja ${serializeCMakeArgs(args)}`;
  await runAsync(cmd, debugLabel);
  await runAsync("ninja", debugLabel);
  process.chdir("../../../..");
};

export var copyRecursiveSync = function (src: string, dest: string) {
  var exists = existsSync(src);
  if (!exists) {
    return;
  }
  var stats = statSync(src);
  var isDirectory = stats.isDirectory();
  if (isDirectory) {
    if (!existsSync(dest)) {
      mkdirSync(dest);
    }
    readdirSync(src).forEach((childItemName) => {
      copyRecursiveSync(
        path.join(src, childItemName),
        path.join(dest, childItemName)
      );
    });
  } else {
    copyFileSync(src, dest);
  }
};

// Cross-platform file operations abstraction
export const fileOps = {
  rm: (p: string) => {
    if (fs.existsSync(p)) {
      fs.rmSync(p, { recursive: true, force: true });
    }
  },

  mkdir: (p: string) => {
    fs.mkdirSync(p, { recursive: true });
  },

  cp: (src: string, dest: string) => {
    copyRecursiveSync(src, dest);
  },

  sed: (file: string, pattern: RegExp, replacement: string) => {
    if (fs.existsSync(file)) {
      const content = fs.readFileSync(file, "utf8");
      const updated = content.replace(pattern, replacement);
      fs.writeFileSync(file, updated, "utf8");
    }
  },
};
