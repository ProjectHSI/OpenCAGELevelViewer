class Patch {
	[Boolean] ShouldPatch([string]$TargetDirectory) { throw("Must override!") }
	[void] ApplyPatch([string]$TargetDirectory) { throw("Must override!") }
}

class GitPatch : Patch {
	[string]$TargetCommit
	[Boolean] ShouldPatch([string]$TargetDirectory) {
		Set-Location $(Join-Path $PSScriptRoot $TargetDirectory)
		$CurrentCommit = (git rev-parse HEAD 2>&1)
		$NotCurrentCommit = $CurrentCommit -ne $this.TargetCommit
		Set-Location -
		return $NotCurrentCommit
	}
}

class GitApplyPatch : GitPatch {
	[string]$Diff

	[void] ApplyPatch([string]$TargetDirectory) {
		Set-Location $(Join-Path $PSScriptRoot $TargetDirectory)
		git am $(Join-Path $PSScriptRoot $this.Diff) >$null 2>&1
		Set-Location -
	}
}

class GitApplyBundle : GitPatch {
	[string]$Bundle

	[void] ApplyPatch([string]$TargetDirectory) {
		Set-Location $(Join-Path $PSScriptRoot $TargetDirectory)
		git bundle verify $(Join-Path $PSScriptRoot $this.Bundle) >$null 2>&1
		if ($LASTEXITCODE -ne 0) {
			Set-Location -
			throw "Bundle is not valid!"
		}
		git pull $(Join-Path $PSScriptRoot $this.Bundle) HEAD >$null 2>&1
		git checkout $this.TargetCommit >$null 2>&1
		Set-Location -
	}
}

$patches = @{
	"ExtMSBuildImports/cryptopp" = [GitApplyBundle]@{ Bundle = 'cryptopp-patch.gitbundle'; TargetCommit = '11893ba12717c3d6590d78f1b52fbd5e237f3e9c' }
}

$wasPatchDone = $false

foreach ($patch in $patches.GetEnumerator()) {
	if ($patch.Value.ShouldPatch($patch.Key)) {
		Write-Host "Patching $($patch.Key)..." -ForegroundColor Green
		$patch.Value.ApplyPatch($patch.Key)
		$wasPatchDone = $true
	} else {
		Write-Host "$($patch.Key) is already patched." -ForegroundColor Yellow
	}
}

if ($wasPatchDone) {
	Write-Host "Patching complete. Please build the solution again." -ForegroundColor Red
} else {
	Write-Host "No patches were applied."
}

Exit $wasPatchDone ? 1 : 0