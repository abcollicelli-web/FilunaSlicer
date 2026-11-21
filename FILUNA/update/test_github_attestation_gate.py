from pathlib import Path
import ast, json, os, subprocess, tempfile, textwrap, unittest
from unittest.mock import patch

workflow = Path(__file__).parents[2] / '.github/workflows/filuna-github-attestation.yml'
code = textwrap.dedent(workflow.read_text(encoding='utf-8').split("python3 - <<'PY'\n", 1)[1].split('          PY\n', 1)[0])
ast.parse(code)

class AttestationGateTests(unittest.TestCase):
    def setUp(self):
        self.sha = 'a' * 40
        self.repo = 'abcollicelli-web/FilunaSlicer'
        self.run = dict(status='completed', conclusion='success', head_sha=self.sha,
            repository={'full_name': self.repo}, head_repository={'full_name': self.repo},
            path='.github/workflows/filuna-native-update-gui-build.yml',
            head_branch='filuna/native-update-first-beta-20261108', html_url='https://github.com/example')
        self.artifacts = {'total_count': 1, 'artifacts': [dict(id=42,
            name='OrcaSlicer_Windows_2.4.0_x64_portable', expired=False, digest='sha256:'+'b'*64)]}

    def execute(self):
        with tempfile.TemporaryDirectory() as tmp:
            previous = os.getcwd()
            try:
                os.chdir(tmp)
                with patch.dict(os.environ, GITHUB_REPOSITORY=self.repo, SOURCE_RUN='123',
                    SOURCE_SHA=self.sha, GITHUB_ENV=str(Path(tmp)/'env')):
                    with patch.object(subprocess, 'check_output', side_effect=[
                        json.dumps(self.run).encode(), json.dumps(self.artifacts).encode()]):
                        exec(compile(code, str(workflow), 'exec'), {})
                return json.loads(Path('source-evidence.json').read_text())
            finally:
                os.chdir(previous)

    def test_valid_import_does_not_claim_release_or_windows_signature(self):
        evidence = self.execute()
        self.assertEqual(evidence['source_sha'], self.sha)
        self.assertEqual(evidence['authenticode'], 'NOT_PERFORMED')
        self.assertEqual(evidence['release_approval'], 'NOT_GRANTED')

    def test_unapproved_source_is_rejected(self):
        for key, value in [('status','in_progress'), ('conclusion','failure'),
            ('head_sha','c'*40), ('head_branch','other'), ('path','other.yml'),
            ('repository',{'full_name':'other/repo'}), ('head_repository',{'full_name':'other/repo'})]:
            with self.subTest(key=key):
                original = self.run[key]
                self.run[key] = value
                with self.assertRaises(SystemExit): self.execute()
                self.run[key] = original

    def test_invalid_artifact_is_rejected(self):
        for key, value in [('expired',True), ('digest',None), ('name','installer.exe')]:
            with self.subTest(key=key):
                artifact = self.artifacts['artifacts'][0]
                original = artifact[key]
                artifact[key] = value
                with self.assertRaises(SystemExit): self.execute()
                artifact[key] = original

    def test_ambiguous_artifact_is_rejected(self):
        self.artifacts['artifacts'] *= 2
        self.artifacts['total_count'] = 2
        with self.assertRaises(SystemExit): self.execute()

if __name__ == '__main__': unittest.main()
