"""Independent end-to-end verification of the offline application; standard library only.

All generated/invalid inputs and exports live in a temporary directory, never in
the historical archive. Existing engines/fixtures are not changed for these tests.
"""
from __future__ import annotations

import argparse
import csv
from fractions import Fraction as F
import hashlib
import io
import json
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
APP: Path
HISTORICAL: Path
WORK: Path
STATS = {'commands': 0, 'historical_scenarios': 0, 'imported_prices': 0,
         'oracle_searches': 0, 'oracle_contracts': 0, 'oracle_evaluations': 0,
         'oracle_qualifiers': 0, 'oracle_outcome_probabilities': 0,
         'historical_prefix_results': 0}


def run(*args, code=0):
    process = subprocess.run([str(APP), *map(str, args)], capture_output=True, text=True,
                             encoding='utf-8', errors='strict', timeout=30)
    STATS['commands'] += 1
    if process.returncode != code:
        raise AssertionError(f'command {args!r}: expected {code}, got {process.returncode}\n{process.stderr}\n{process.stdout[:3000]}')
    return process


def document(*args, code=0):
    return json.loads(run(*args, '--json', code=code).stdout)


def write(name, content):
    path = WORK / name
    path.write_text(content, encoding='utf-8', newline='\n')
    return path


def fraction_text(value):
    return f'{F(value).numerator}/{F(value).denominator}'


def catalog(data, label='Synthetic, "quoted" collection'):
    rows = ['tradeup-catalog-v1', 'scenario\tfixture\tExplicit synthetic fixture',
            'valuation\tfixture-valuations-v1', 'unit\tfixture-unit']
    for c in data['collections']:
        rows.append(f'collection\t{c["id"]}\t{label}')
        for i, p in enumerate(c['prices']):
            rows.append(f'outcome\t{c["id"]}\to{i:02}\t{p}\tsynthetic\tIndependent fixture')
    for b in data['buckets']:
        rows.append(f'bucket\t{b["id"]}\t{b["collection"]}\t{b["wear"]}\t{b["cost"]}\tsynthetic\tIndependent fixture\trepresentative\tBucket {b["id"]}')
    return '\n'.join(rows) + '\n'


def values(result):
    return [F(result[key]) for key in ('cost', 'gross_ev', 'net_ev', 'return_percent', 'roi_percent', 'expected_profit', 'worst_net')]


def canonical_identity_from_inspection(d, source_binding=''):
    c = d['dataset']
    fields = [c['format'], c['mapping'], source_binding, c['scenario'], c['scenario_label'], c['probability_policy'],
              c['fee_policy'], c['valuation_policy'], c['price_unit'], str(len(d['collections']))]
    def quote(q):
        return [q['value'], q['kind'], q['source']]
    for collection in d['collections']:
        fields += [collection['id'], collection['label'], str(len(collection['outcomes']))]
        for o in collection['outcomes']:
            fields += [o['id'], *quote(o['price'])]
    fields += [str(len(d['buckets']))]
    for b in d['buckets']:
        fields += [b['id'], b['label'], b['collection'], 'L' if b['wear']=='lower' else 'H', *quote(b['unit_cost'])]
    fields += [str(len(c['limitations'])), *c['limitations']]
    payload = b'tradeup-dataset-canonical-v1\n' + b''.join(str(len(s.encode())).encode()+b':'+s.encode() for s in fields)
    return hashlib.sha256(payload).hexdigest(), hashlib.sha256(b'tradeup-snapshot-v1\n'+payload).hexdigest()


class CliVerification(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.matrix_path = HISTORICAL / '2024/most recent version i think/input.txt'
        cls.matrix_text = cls.matrix_path.read_text()
        cls.matrix = [list(map(F, line.split())) for line in cls.matrix_text.splitlines()[1:]]
        cls.scenarios = json.loads((ROOT / 'tests/fixtures/scenarios.json').read_text())['scenarios']
        cls.equal = ROOT / 'examples/synthetic-equal-returns.tsv'
        cls.boundary = ROOT / 'examples/synthetic-boundary.tsv'

    def generic(self, command, path=None, *args, code=0):
        return document(command, '--input', path or self.equal, '--format', 'catalog-v1', *args, code=code)

    def test_01_help_profiles_and_option_validation(self):
        self.assertIn('entirely offline', run('--help').stdout)
        profiles = document('profiles')
        self.assertEqual([(p['lower'], p['higher']) for p in profiles], [(5, 5), (6, 4)])
        for args in [(), ('unknown',), ('search',), ('profiles', '--top', '1'),
                     ('inspect', '--input'), ('profiles', '--json', '--json')]:
            self.assertTrue(run(*args, code=2).stderr)

    def test_02_scenarios_against_saved_source(self):
        scenarios = document('scenarios')
        source = (HISTORICAL / '2024/most recent version i think/main.cpp').read_text()
        calls = re.findall(r'strcpy\(raritate,"([^"]+)"\);\s*programel\(n,(.*?),raritate\);', source)
        self.assertEqual(len(scenarios), 12)
        for actual, expected, (label, args) in zip(scenarios, self.scenarios, calls):
            rows = [int(x) for x in re.findall(r'sefu\[(\d+)-67\+1\]', args)]
            self.assertEqual(actual['label'], label)
            self.assertEqual(actual['workbook_output_rows'] + [actual['workbook_cost_row']], rows)
            self.assertEqual(rows, expected['workbook_output_rows'] + [expected['workbook_cost_row']])

    def test_03_preserve_all_3190_prices_and_source_digest(self):
        d = document('inspect', '--input', self.matrix_path, '--matrix')
        self.assertEqual([list(map(F, row)) for row in d['matrix']], self.matrix)
        self.assertEqual((d['matrix_rows'], d['matrix_columns']), (55, 58))
        self.assertEqual(d['dataset']['source_sha256'], hashlib.sha256(self.matrix_path.read_bytes()).hexdigest())
        self.assertTrue(d['dataset']['matches_preserved_matrix_values'])
        self.assertIsNone(d['dataset']['currency'])
        self.assertIsNone(d['dataset']['quote_timestamp'])
        self.assertEqual((d['dataset']['content_sha256'],d['dataset']['snapshot_id']),canonical_identity_from_inspection(d,d['dataset']['source_binding']))
        STATS['imported_prices'] = sum(map(len, d['matrix']))

    def test_04_all_twelve_imported_scenarios_and_exact_evaluations(self):
        for scenario, expected in zip(document('scenarios'), self.scenarios):
            with self.subTest(scenario=scenario['id']):
                d = document('inspect', '--input', self.matrix_path, '--scenario', scenario['id'])
                self.assertEqual(len(d['buckets']), 58)
                self.assertEqual(len(d['collections']), 29)
                for i, b in enumerate(d['buckets']):
                    self.assertEqual(F(b['unit_cost']['value']), self.matrix[expected['workbook_cost_row']-67][i])
                    self.assertEqual(b['wear'], 'lower' if i%2==0 else 'higher')
                    self.assertEqual(b['collection'], f'c{i//2+1:02}')
                    self.assertEqual(b['unit_cost']['kind'], 'minimum_derived')
                for i, c in enumerate(d['collections']):
                    wanted = {f'row{r}': self.matrix[r-67][2*i+1] for r in expected['workbook_output_rows'] if self.matrix[r-67][2*i+1]}
                    self.assertEqual({o['id']: F(o['price']['value']) for o in c['outcomes']}, wanted)
                e = document('eval', '--input', self.matrix_path, '--scenario', scenario['label'], '--contract', 'b002:5,b029:5', '--details')['results'][0]
                costs = [self.matrix[expected['workbook_cost_row']-67][b-1] for b in (2, 29)]
                outputs = [[self.matrix[r-67][((b-1)//2)*2+1] for r in expected['workbook_output_rows']
                            if self.matrix[r-67][((b-1)//2)*2+1]] for b in (2, 29)]
                cost=5*sum(costs); denominator=5*sum(map(len,outputs)); gross=F(5,denominator)*sum(map(sum,outputs)); net=gross*F(87,100)
                self.assertEqual(values(e), [cost,gross,net,100*net/cost,100*net/cost-100,net-cost,min(map(min,outputs))*F(87,100)])
                self.assertEqual(sum(F(o['probability']) for o in e['outcomes']),1)
                STATS['historical_scenarios'] += 1; STATS['oracle_evaluations'] += 1

    def test_05_historical_replay_application_path(self):
        d = document('replay', '--input', self.matrix_path, '--details')
        e = d['results'][0]
        self.assertEqual(values(e), [F('4.85'),F(6427,900),F(186383,30000),F(186383,1455),F(40883,1455),F(40883,30000),F('2.784')])
        self.assertEqual([(a['bucket'],a['quantity']) for a in e['allocations']], [('b002',5),('b029',5)])
        self.assertEqual(len(e['outcomes']),9)
        self.assertTrue(all(F(o['probability'])==F(1,9) for o in e['outcomes']))
        text = run('replay','--input',self.matrix_path).stdout
        self.assertIn('128.098282',text); self.assertIn('28.098282',text)
        self.assertLess(F(e['worst_net']),F(e['cost']))

    def test_06_dimensions_syntax_and_missing_prices(self):
        lines=self.matrix_text.splitlines()
        variants=[['57',*lines[1:]],lines[:-1],[*lines,'0'],['58 55',*lines[1:]]]
        tokens=lines[1].split()
        for token in ('NA','nan','inf','1e2','-1','1/2','1.','1..2','9223372036854775808'):
            variants.append([lines[0],' '.join([token,*tokens[1:]]),*lines[2:]])
        variants.append([lines[0],' '.join(tokens[:-1]),*lines[2:]])
        variants.append([lines[0],' '.join([*tokens,'1']),*lines[2:]])
        for i, variant in enumerate(variants):
            path=write(f'invalid-matrix-{i}.txt','\n'.join(variant)+'\n')
            self.assertTrue(run('inspect','--input',path,code=2).stderr)
        oversized=write('oversized.txt',' '*(1024*1024+1))
        self.assertIn('byte limit',run('inspect','--input',oversized,code=2).stderr)

    def test_07_padding_is_metadata_and_real_zero_is_retained(self):
        lines=self.matrix_text.splitlines(); row=lines[20].split(); row[1]='1';lines[20]=' '.join(row) # workbook row 86, breakout padding
        path=write('nonzero-padding.txt','\n'.join(lines)+'\n')
        self.assertIn('padding',run('inspect','--input',path,code=2).stderr)
        lines=self.matrix_text.splitlines(); row=lines[16].split(); row[1]='0';lines[16]=' '.join(row) # workbook row 82, an actual modeled slot
        path=write('zero-modeled.txt','\n'.join(lines)+'\n')
        d=document('inspect','--input',path)
        breakout=d['collections'][0]['outcomes'];self.assertEqual(len(breakout),4)
        self.assertEqual(F(breakout[0]['price']['value']),0)
        self.assertEqual(breakout[0]['price']['kind'],'unknown')
        self.assertFalse(d['dataset']['matches_preserved_matrix_values'])
        self.assertIn('unchanged',run('replay','--input',path,code=2).stderr)

    def test_08_composite_ratio_and_minimum_provenance(self):
        for scenario in ('normal-ft-mw-mov','st-ft-mw-mov','normal-mw-fn-mov','st-mw-fn-mov'):
            d=document('inspect','--input',self.matrix_path,'--scenario',scenario)
            output_row=next(s for s in d['scenarios'] if s['id']==scenario)['workbook_output_rows'][-1]
            composite=next(o for o in d['collections'][21]['outcomes'] if o['id']==f'row{output_row}')['price']
            self.assertEqual(composite['kind'],'composite_estimate')
            self.assertIn('AVERAGE(',composite['source'])
            self.assertIn('MIN(',d['buckets'][1]['unit_cost']['source'])
        d=document('inspect','--input',self.matrix_path,'--scenario','normal-ft-mw-rosu')
        self.assertEqual(d['collections'][2]['outcomes'][0]['price']['kind'],'ratio_estimate')
        self.assertIn('/4',d['collections'][2]['outcomes'][0]['price']['source'])
        e=document('eval','--input',self.matrix_path,'--scenario','normal-ft-mw-mov','--contract','b043:5,b044:5')['results'][0]
        self.assertIn('composite_estimate',e['outcome_quote_kinds'])
        self.assertTrue(any('Estimated/composite' in s for s in e['limitations']))

    def test_09_identity_paths_formatting_and_changed_unused_price(self):
        a=document('inspect','--input',self.matrix_path)['dataset']
        copy=write('same values with spaces.txt','58\n'+'\n'.join('  '.join(line.split()) for line in self.matrix_text.splitlines()[1:])+'\n')
        b=document('inspect','--input',copy)['dataset']
        self.assertEqual(a['snapshot_id'],b['snapshot_id']);self.assertEqual(a['content_sha256'],b['content_sha256'])
        self.assertNotEqual(a['source_sha256'],b['source_sha256'])
        lines=self.matrix_text.splitlines(); row=lines[1].split();row[0]='0.12';lines[1]=' '.join(row)
        changed=write('changed-unused-row.txt','\n'.join(lines)+'\n')
        self.assertNotEqual(a['snapshot_id'],document('inspect','--input',changed)['dataset']['snapshot_id'])

    def test_10_independent_sha256_canonicalization_and_order(self):
        content=self.equal.read_text(encoding='utf-8')
        for i in range(8):
            rows=content.splitlines(); header=rows.pop(0); random.Random(i).shuffle(rows)
            text=header+'\n'+'\n'.join(rows)+'\n'+'#'+('a'*i)+'\n'
            path=write(f'canonical-{i}.tsv',text)
            d=self.generic('inspect',path)
            expected=canonical_identity_from_inspection(d)
            self.assertEqual((d['dataset']['content_sha256'],d['dataset']['snapshot_id']),expected)
            self.assertEqual(d['dataset']['source_sha256'],hashlib.sha256(path.read_bytes()).hexdigest())
            self.assertEqual(d['dataset']['snapshot_id'],self.generic('inspect')['dataset']['snapshot_id'])

    def test_11_contract_selection_context_and_profiles(self):
        e=self.generic('eval',None,'--contract','b002:2,b001:5,b002:3')['results'][0]
        again=self.generic('eval',None,'--contract',e['contract_id'])['results'][0]
        self.assertEqual(e,again)
        self.assertTrue(run('eval','--format','catalog-v1','--input',self.boundary,'--contract',e['contract_id'],code=2).stderr)
        for args in [('--contract','unknown:10'),('--contract','b001:9'),('--contract','b001:0,b004:10')]:
            self.assertTrue(run('eval','--input',self.equal,'--format','catalog-v1',*args,code=2).stderr)
        for args in [('--profile','unrestricted'),('--scenario','unknown'),('--min-roi','-101'),('--min-return','115','--min-roi','15'),('--max-cost','0')]:
            self.assertTrue(run('search','--input',self.equal,'--format','catalog-v1',*args,code=2).stderr)

    def test_12_equal_return_contracts_deterministic_ties_and_top(self):
        d=self.generic('search',None,'--top','1')
        self.assertTrue(d['completion']['complete'])
        self.assertEqual(len(d['results']),405)
        ids=[r['contract_id'] for r in d['results']]
        self.assertEqual(len(set(ids)),405);self.assertEqual(ids,sorted(ids))
        self.assertEqual({F(r['return_percent']) for r in d['results']},{F(870)})
        text=run('search','--input',self.equal,'--format','catalog-v1','--top','1').stdout
        self.assertIn('retained: 405; matching post-search filter: 405; shown: 1',text)
        self.assertEqual(len(self.generic('search',None,'--profile','6/4')['results']),390)

    def test_13_filters_inclusive_cost_and_complete_empty(self):
        d=self.generic('search',None,'--max-cost','10','--sort','profit');self.assertEqual(len(d['results']),405)
        d=self.generic('search',None,'--max-cost','9')
        self.assertEqual(len(d['results']),0);self.assertEqual(d['selection']['retained_qualifying'],405)
        self.assertEqual(d['selection']['filtered_out'],405);self.assertTrue(d['completion']['complete'])
        text=run('search','--input',self.boundary,'--format','catalog-v1').stdout
        self.assertIn('No qualifying contracts in the complete selected search domain',text)

    def test_14_exact_threshold_boundaries(self):
        single=self.generic('eval',self.boundary,'--contract','b001:5,b002:5')
        self.assertEqual(F(single['results'][0]['return_percent']),115)
        self.assertIsNone(single['selection']['retained_qualifying'])
        self.assertEqual(len(self.generic('search',self.boundary)['results']),0)
        self.assertEqual(len(self.generic('search',self.boundary,'--min-return','114.999999999999')['results']),1)
        self.assertEqual(len(self.generic('search',self.boundary,'--min-return','115.000000000001')['results']),0)
        self.assertEqual(len(self.generic('search',self.boundary,'--min-roi','14.999999999999')['results']),1)

    def test_15_python_cartesian_fraction_search_oracle(self):
        import verify_reference as oracle
        for seed in range(8):
            data=oracle.dataset(1+seed%3,1+(seed//3)%3,seed+320)
            path=write(f'oracle-{seed}.tsv',catalog(data))
            for lower,higher in ((5,5),(6,4)):
                for ranking in ('roi','profit'):
                    actual=self.generic('search',path,'--profile',f'{lower}/{higher}','--sort',ranking,'--details')
                    data['snapshot']=actual['dataset']['snapshot_id'];data['scenario']='fixture'
                    all_vectors=oracle.cartesian_oracle(data,lower,higher)
                    wanted={k:oracle.arithmetic_oracle(data,q) for k,q in all_vectors.items() if oracle.arithmetic_oracle(data,q)[0][3]>115}
                    found={r['contract_id']:r for r in actual['results']}
                    self.assertEqual(set(found),set(wanted))
                    for key,(expected,probabilities) in wanted.items():
                        e=found[key];self.assertEqual(values(e),expected)
                        self.assertEqual({(o['collection'],o['outcome']):(F(o['probability']),F(o['gross_price']['value']),F(o['net_receipt'])) for o in e['outcomes']},probabilities)
                        STATS['oracle_outcome_probabilities']+=len(probabilities)
                    index=4 if ranking=='roi' else 5
                    self.assertEqual([r['contract_id'] for r in actual['results']],sorted(wanted,key=lambda k:(-wanted[k][0][index],k)))
                    STATS['oracle_searches']+=1;STATS['oracle_contracts']+=len(all_vectors);STATS['oracle_qualifiers']+=len(wanted)

    def test_16_arbitrary_outcome_counts_zero_and_missing(self):
        data={'collections':[{'id':'c0','prices':list(map(F,range(7)))}],
              'buckets':[{'id':'b0','collection':'c0','wear':'L','cost':F(1)}, {'id':'b1','collection':'c0','wear':'H','cost':F(1)}]}
        path=write('seven-outcomes.tsv',catalog(data))
        e=self.generic('eval',path,'--contract','b0:6,b1:4','--details')['results'][0]
        self.assertEqual(len(e['outcomes']),7);self.assertEqual(F(e['gross_ev']),3)
        self.assertTrue(all(F(o['probability'])==F(1,7) for o in e['outcomes']))
        self.assertEqual(F(e['outcomes'][0]['gross_price']['value']),0)
        for old,new in [('o00\t0\t','o00\t\t'),('o00\t0\t','o00\tNA\t')]:
            broken=write('missing-outcome.tsv',path.read_text().replace(old,new))
            self.assertTrue(run('inspect','--input',broken,'--format','catalog-v1',code=2).stderr)

    def test_17_budget_statuses_and_retained_partial_results(self):
        for args,status in [(('--max-evaluations','1','--no-pruning'),'evaluation-budget'),(('--max-work','1'),'work-budget'),
                            (('--max-results','1'),'result-budget'),(('--max-result-bytes','1024'),'result-budget')]:
            d=self.generic('search',None,*args,code=3)
            self.assertFalse(d['completion']['complete']);self.assertEqual(d['completion']['status'],status)
            if status=='result-budget':
                self.assertEqual(d['statistics']['qualifying_found'],len(d['results'])+1)
            if args[0]=='--max-result-bytes':self.assertLessEqual(d['statistics']['result_capacity_bytes'],1024)
        d=document('search','--input',self.matrix_path,'--time-ms','1','--max-evaluations','10000000','--max-work','50000000','--max-results','100000',code=3)
        self.assertEqual(d['completion']['status'],'time-budget')

    def test_18_arithmetic_statuses(self):
        content=self.boundary.read_text()
        huge=write('huge-outcome.tsv',content.replace('o\t1.15\t','o\t9223372036854775807\t'))
        self.assertIn('Unsupported arithmetic',run('search','--input',huge,'--format','catalog-v1',code=5).stderr)
        costs=write('huge-cost.tsv',content.replace('0.087','9223372036854775807'))
        d=self.generic('search',costs,'--no-pruning',code=5)
        self.assertEqual(d['completion']['status'],'arithmetic-limit');self.assertFalse(d['completion']['complete'])

    def test_19_export_json_csv_exact_fields_and_status(self):
        js=WORK/'search-export.json';cs=WORK/'search-export.csv'
        d=self.generic('search',None,'--top','1','--max-results','3','--output',js,'--details',code=3)
        saved=json.loads(js.read_text(encoding='utf-8'));self.assertEqual(saved,d)
        self.generic('search',None,'--top','1','--max-results','3','--output',cs,'--details',code=3)
        rows=list(csv.DictReader(io.StringIO(cs.read_text(encoding='utf-8'))))
        self.assertEqual(rows[0]['record_type'],'metadata')
        metadata=json.loads(rows[0]['metadata_json']);self.assertEqual(metadata['completion']['status'],'result-budget')
        self.assertEqual(len(rows)-1,3)
        for row,result in zip(rows[1:],d['results']):
            self.assertEqual(row['contract_id'],result['contract_id']);self.assertEqual(row['evaluation_id'],result['evaluation_id'])
            self.assertEqual([F(row[k]) for k in ('cost','gross_ev','net_ev','return_percent','roi_percent','expected_profit','worst_net')],values(result))
            self.assertEqual(json.loads(row['allocations_json']),result['allocations']);self.assertEqual(json.loads(row['outcomes_json']),result['outcomes'])
        empty=WORK/'empty.csv';self.generic('search',self.boundary,'--output',empty)
        rows=list(csv.DictReader(io.StringIO(empty.read_text())))
        self.assertEqual(len(rows),1);self.assertTrue(json.loads(rows[0]['metadata_json'])['completion']['complete'])

    def test_20_output_failure_no_overwrite_and_cleanup(self):
        existing=write('existing.json','keep these bytes')
        self.generic_error_output(existing)
        self.assertEqual(existing.read_text(),'keep these bytes')
        target=WORK/'too-small.json'
        self.assertTrue(run('replay','--input',self.matrix_path,'--output',target,'--max-export-bytes','10',code=4).stderr)
        self.assertFalse(target.exists());self.assertFalse(list(WORK.glob('.tradeup-export-*.tmp')))
        protected=WORK/'historical';protected.mkdir(exist_ok=True)
        self.generic_error_output(protected/'forbidden.json');self.assertFalse((protected/'forbidden.json').exists())
        self.generic_error_output(WORK/'does-not-exist'/'report.json')

    def generic_error_output(self,path):
        self.assertTrue(run('search','--input',self.equal,'--format','catalog-v1','--output',path,code=4).stderr)

    def test_21_windows_spaces_and_unicode_paths(self):
        directory=WORK/'path with spaces and ș';directory.mkdir(exist_ok=True)
        copy=directory/'historical price matrix.txt';copy.write_bytes(self.matrix_path.read_bytes())
        output=directory/'replay results.json'
        d=document('replay','--input',copy,'--output',output,'--details')
        self.assertEqual(json.loads(output.read_text(encoding='utf-8')),d)
        self.assertEqual(F(d['results'][0]['return_percent']),F(186383,1455))

    def test_22_invalid_catalog_encoding_structure_and_metadata(self):
        content=self.boundary.read_text()
        variants=[content.replace('tradeup-catalog-v1','unknown-format'),content+'snapshot\tforged\n',
                  content.replace('L\t0.087','Q\t0.087'),content+'collection\tc\tduplicate\n',
                  content.replace('synthetic\tLocal synthetic boundary','composite_estimate\t'),
                  content.replace('outcome\tc\to\t1.15\tsynthetic\tLocal synthetic boundary\n',''),
                  content.replace('bucket\tb002\tc','bucket\tb002\tunknown'),
                  content+'outcome\tc\to\t1.15\tsynthetic\tduplicate identity\n']
        for i,v in enumerate(variants):
            self.assertTrue(run('inspect','--input',write(f'bad-catalog-{i}.tsv',v),'--format','catalog-v1',code=2).stderr)
        for raw in (b'\x00',b'\xc0\xaf',b'\xed\xa0\x80',b'\xf4\x90\x80\x80'):
            path=WORK/'invalid-bytes.tsv';path.write_bytes(raw)
            self.assertTrue(run('inspect','--input',path,'--format','catalog-v1',code=2).stderr)

    def test_23_safe_resource_caps_and_irrelevant_options(self):
        for args in [('--max-evaluations','10000001'),('--max-work','50000001'),('--time-ms','60001'),('--max-results','100001'),
                     ('--max-result-bytes','67108865'),('--max-result-bytes','1'),('--top','1001'),('--max-work','0'),('--sort','unknown')]:
            self.assertTrue(run('search','--input',self.equal,'--format','catalog-v1',*args,code=2).stderr)
        for args in [('replay','--contract','b001:10'),('eval','--top','1'),('inspect','--details')]:
            self.assertTrue(run(*args,'--input',self.matrix_path,code=2).stderr)

    def test_24_utf8_and_quoted_csv_fields_and_fraction_precision(self):
        data={'collections':[{'id':'c0','prices':[F(115,101)]}],
              'buckets':[{'id':'b0','collection':'c0','wear':'L','cost':F(1,37)},
                         {'id':'b1','collection':'c0','wear':'H','cost':F(1,37)}]}
        path=write('quoted-unicode.tsv',catalog(data,'Collection:1, ș "quoted"').replace('Independent fixture','Copied, "quoted" source'))
        d=self.generic('inspect',path)
        self.assertEqual((d['dataset']['content_sha256'],d['dataset']['snapshot_id']),canonical_identity_from_inspection(d))
        output=WORK/'quoted-unicode.csv'
        d=self.generic('search',path,'--output',output,'--details')
        rows=list(csv.DictReader(io.StringIO(output.read_text(encoding='utf-8'))))
        self.assertEqual(len(rows),2)
        self.assertEqual(json.loads(rows[1]['allocations_json']),d['results'][0]['allocations'])
        self.assertEqual(json.loads(rows[1]['outcomes_json']),d['results'][0]['outcomes'])
        self.assertEqual(F(rows[1]['cost']),F(10,37))

    def test_25_authentic_historical_bounded_search_fraction_oracle(self):
        for profile in ('5/5','6/4'):
            d=document('search','--input',self.matrix_path,'--profile',profile,'--max-evaluations','1000','--details',code=3)
            self.assertFalse(d['completion']['complete']);self.assertEqual(d['completion']['status'],'evaluation-budget')
            self.assertEqual(d['statistics']['candidates_evaluated'],1000)
            self.assertGreater(len(d['results']),0)
            self.assertEqual(len(d['results']),len({e['contract_id'] for e in d['results']}))
            for e in d['results']:
                counts={};cost=F(0);lower=higher=0
                for a in e['allocations']:
                    b=int(a['bucket'][1:])-1;q=a['quantity'];c=b//2
                    cost+=q*self.matrix[1][b];counts[c]=counts.get(c,0)+q
                    if b%2: higher+=q
                    else: lower+=q
                self.assertEqual((lower,higher),tuple(map(int,profile.split('/'))))
                prices={(c,r):self.matrix[r-67][2*c+1] for c in counts for r in range(82,87) if self.matrix[r-67][2*c+1]}
                tickets=sum(counts[c] for c,r in prices)
                gross=sum(F(counts[c],tickets)*p for (c,r),p in prices.items());net=gross*F(87,100)
                self.assertEqual(values(e),[cost,gross,net,100*net/cost,100*net/cost-100,net-cost,min(prices.values())*F(87,100)])
                self.assertGreater(F(e['return_percent']),115)
                expected={(f'c{c+1:02}',f'row{r}'):(F(counts[c],tickets),p,p*F(87,100)) for (c,r),p in prices.items()}
                actual={(o['collection'],o['outcome']):(F(o['probability']),F(o['gross_price']['value']),F(o['net_receipt'])) for o in e['outcomes']}
                self.assertEqual(actual,expected)
                STATS['historical_prefix_results']+=1

    def test_26_file_record_and_entity_limits(self):
        for name, size, format in [('large-matrix.txt',1024*1024+1,'historical-2024'),
                                   ('large-catalog.tsv',8*1024*1024+1,'catalog-v1')]:
            path=WORK/name
            path.write_bytes(b'x'*size)
            self.assertIn('byte limit',run('inspect','--input',path,'--format',format,code=2).stderr)
        content=self.boundary.read_text(encoding='utf-8')
        variants=[(content+'limitation\t'+'x'*16384+'\n','16 KiB'),
                  (content+''.join(f'collection\tc{i}\tC\n' for i in range(4097)),'4096'),
                  (content+''.join(f'bucket\tb{i+10}\tc\tL\t1\tsynthetic\tsource\trepresentative\tB\n' for i in range(4097)),'4096'),
                  (content+''.join(f'outcome\tc\to{i}\t1\tsynthetic\tsource\n' for i in range(100001)),'100000')]
        for i,(text,diagnostic) in enumerate(variants):
            error=run('inspect','--input',write(f'entity-limit-{i}.tsv',text),'--format','catalog-v1',code=2).stderr
            self.assertIn(diagnostic,error)

    def test_27_newline_identity_and_control_characters(self):
        raw=self.equal.read_bytes().replace(b'\r\n',b'\n')
        paths=[WORK/'lf.tsv',WORK/'crlf.tsv']
        paths[0].write_bytes(raw);paths[1].write_bytes(raw.replace(b'\n',b'\r\n'))
        a,b=[self.generic('inspect',p)['dataset'] for p in paths]
        self.assertEqual(a['snapshot_id'],b['snapshot_id'])
        self.assertNotEqual(a['source_sha256'],b['source_sha256'])
        for i,raw in enumerate((raw.replace(b'collection\t',b'collection\r\t',1),raw+b'\r',raw+b'\x1b',raw+b'\x7f',raw+b'\xe2\x82')):
            path=WORK/f'controls-{i}.tsv';path.write_bytes(raw)
            self.assertTrue(run('inspect','--input',path,'--format','catalog-v1',code=2).stderr)

    def test_28_existing_hard_link_is_never_modified(self):
        original=write('hardlink-original.json','original bytes')
        target=WORK/'hardlink-output.json'
        os.link(original,target)
        self.generic_error_output(target)
        self.assertEqual(original.read_bytes(),b'original bytes')
        self.assertEqual(target.read_bytes(),b'original bytes')

    def test_29_symlink_destinations_and_protected_parent(self):
        directory=WORK/'symlink-tests';directory.mkdir()
        missing=directory/'missing.json';link=directory/'dangling.json'
        try:
            link.symlink_to(missing)
        except OSError as error:
            if os.name=='nt' and getattr(error,'winerror',None)==1314:
                self.skipTest('Windows account lacks symlink privilege; Linux CI runs these cases')
            raise
        error=run('search','--input',self.equal,'--format','catalog-v1','--output',link,code=4)
        self.assertIn('output already exists',error.stderr)
        self.assertTrue(link.is_symlink());self.assertFalse(missing.exists())
        original=write('symlink-original.json','keep original')
        existing=directory/'existing.json';existing.symlink_to(original)
        self.generic_error_output(existing);self.assertEqual(original.read_text(),'keep original')
        protected=directory/'historical';protected.mkdir()
        alias=directory/'alias';alias.symlink_to(protected,target_is_directory=True)
        error=run('search','--input',self.equal,'--format','catalog-v1','--output',alias/'forbidden.json',code=4)
        self.assertIn('resolved output is inside protected',error.stderr)
        self.assertEqual(list(protected.iterdir()),[])

    @unittest.skipUnless(os.name=='nt','Windows junction behavior only')
    def test_30_windows_junction_resolves_protected_directory(self):
        directory=WORK/'junction-tests';directory.mkdir()
        protected=directory/'historical';protected.mkdir()
        alias=directory/'innocent-alias'
        created=subprocess.run(['cmd','/d','/c','mklink','/J',str(alias),str(protected)],
                               capture_output=True,timeout=10)
        self.assertEqual(created.returncode,0,created.stderr)
        error=run('search','--input',self.equal,'--format','catalog-v1','--output',alias/'forbidden.json',code=4)
        self.assertIn('resolved output is inside protected',error.stderr)
        self.assertEqual(list(protected.iterdir()),[])
        allowed=directory/'allowed';allowed.mkdir()
        safe_alias=directory/'safe-alias'
        created=subprocess.run(['cmd','/d','/c','mklink','/J',str(safe_alias),str(allowed)],
                               capture_output=True,timeout=10)
        self.assertEqual(created.returncode,0,created.stderr)
        replay=document('replay','--input',self.matrix_path,'--output',safe_alias/'report.json')
        self.assertEqual(json.loads((allowed/'report.json').read_text(encoding='utf-8')),replay)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app',type=Path,required=True)
    parser.add_argument('--historical-root',type=Path,default=ROOT/'historical')
    args=parser.parse_args();APP=args.app.resolve();HISTORICAL=args.historical_root.resolve()
    if not APP.is_file():parser.error('compiled application not found')
    with tempfile.TemporaryDirectory(prefix='tradeup-cli-tests-') as temporary:
        WORK=Path(temporary)
        outcome=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(CliVerification))
    print('Independent application checks:',json.dumps(STATS,sort_keys=True))
    raise SystemExit(0 if outcome.wasSuccessful() else 1)
