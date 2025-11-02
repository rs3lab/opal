#!/usr/bin/env python3

from math import prod
import os
import subprocess
import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import sys

NUM_RECORDS = [100_000_000]
NUM_RECORD_LABELS = ['100M']
SKEW = [0.1] #0.001, 0.01, 0.1, 0.2, 0.5]
SECONDS = 10
SERVER = 'srv9'
NUM_REPLICATES = 5

if SERVER == 'srv1':
    threads = [1, 2, 4, 8, 16, 28, 40, 56, 70, 84, 112, 128, 168, 224]
    num_threads_per_socket = 28
    maxcpuid = 223
elif SERVER == 'srv4':
    threads = [1, 2, 5, 10, 20, 28, 35, 40, 56]
    num_threads_per_socket = 28
elif SERVER == 'srv9' or SERVER == 'srv10':
    threads = [1, 2, 4, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96]
    num_threads_per_socket = 48
    maxcpuid = 95
elif SERVER == 'srv8':
    threads = [1, 2, 4, 8, 16, 32, 64, 96, 112, 128]
    num_threads_per_socket = 64
elif SERVER == 'azure':
    threads = [1, 2, 4, 8, 16, 32, 48, 56, 64]
else:
    print("Unknown server")
    exit(-1)

pd.options.display.max_columns = None
pd.options.display.max_rows = None

base_repo_dir = os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))))
sys.path.append(base_repo_dir)
from common.numa import NUM_SOCKETS, NUM_CORES

base_data_dir = os.path.join(base_repo_dir, 'plots', 'index', 'nopinning-data')
data_dir = base_data_dir

class PiBenchExperiment:
    opTypes = ['Insert', 'Read', 'Update', 'Remove', 'Scan']
    finishTypes = ['completed', 'succeeded']

    def __init__(self, exp_name, index, wrapper_bin, dense, **kwargs):
        self.name = exp_name
        self.index = index
        self.wrapper_bin = wrapper_bin
        self.pibench_args = []
        self.results = []
        kwargs['skip_verify'] = False
        kwargs['apply_hash'] = not dense

        if "latency" in base_data_dir:
            kwargs["latency_sampling"] = 0.01

        if "otdl" in index:
            kwargs["num_delegation_threads"] = 1
            kwargs["num_threads_per_socket"] = num_threads_per_socket

        self.kwargs = kwargs
        for k, w in kwargs.items():
            self.pibench_args.append('--{}={}'.format(k, w))

        def get_numactl_command(threads):
            upto = (threads + NUM_CORES - 1) // NUM_CORES
            upto = min(upto, NUM_SOCKETS)
            sockets = ','.join(map(str, range(upto)))
            maxcore = min(threads - 1, maxcpuid)
            return ['taskset', '-c', f'0-{maxcore}','numactl', f'--membind={sockets}']

        self.numactl = get_numactl_command(kwargs['threads'])

    def run(self, ith, idx, total):
        commands = [*self.numactl, PiBenchExperiment.pibench_bin,
                    self.wrapper_bin, *self.pibench_args]
        commands.append('--bulk_load')
        commands.append('--disable_pinning')
        command_str = f'Executing ({idx}/{total}):' + ' '.join(commands)
        print(command_str)
        result_text = None
        skipped = False
        if os.path.exists(os.path.join(data_dir, f'{self.name}.raw', f'{self.index}-{self.kwargs["threads"]}-{ith}.out')):
            skipped = True
            print('Skipping')
        result = None
        if not skipped:
            try:
                result = subprocess.run(
                    commands, capture_output=True, text=True,
                    timeout=5*60
                )
                result_text = result.stdout
            except subprocess.TimeoutExpired as e:
                skipped = True
                print(e)
                print(f'\n\nTIMED OUT after {e.timeout} seconds for command \n  "{command_str}"')
                result_text = ''
        else:
            with open(os.path.join(data_dir, f'{self.name}.raw', f'{self.index}-{self.kwargs["threads"]}-{ith}.out'), 'r') as f:
                result_text = f.read()

        def parse_output(text):
            results = pd.DataFrame(
                index=PiBenchExperiment.opTypes, columns=PiBenchExperiment.finishTypes)
            for opType in PiBenchExperiment.opTypes:
                for finishType in PiBenchExperiment.finishTypes:
                    pattern = r'\s+\-\s{}\s{}\:\s(.+)\sops'.format(
                        opType, finishType)
                    count = 0
                    for line in text.split('\n'):
                        m = re.match(pattern, line)
                        if m:
                            if "btreeomix" in self.wrapper_bin and count == 0:
                                count = 1
                                continue
                            throughput = float(m.group(1))
                            print('{} {} throughput: {}'.format(
                                opType, finishType, throughput))
                            results.loc[opType, finishType] = throughput
                            break
                    else:
                        print(
                            'Warning - {} {} throughput not found'.format(opType, finishType))
                        results.loc[opType, finishType] = float('NaN')

            return results

        self.results.append(parse_output(result_text))
        assert(ith == len(self.results))
        if not skipped:
            with open(os.path.join(data_dir, f'{self.name}.raw', f'{self.index}-{self.kwargs["threads"]}-{ith}.out'), 'w') as f:
                f.writelines(result.stdout)
            with open(os.path.join(data_dir, f'{self.name}.raw', f'{self.index}-{self.kwargs["threads"]}-{ith}.err'), 'w') as f:
                f.writelines(result.stderr)


gl_df_columns = ['exp', 'name'] + [f'{opType}-ratio' for opType in PiBenchExperiment.opTypes] + ['key-type', 'distribution', 'skew-factor'] + ['index', 'thread', 'replicate'] + PiBenchExperiment.finishTypes + [
    f'{opType}-{finishType}' for opType in PiBenchExperiment.opTypes for finishType in PiBenchExperiment.finishTypes]

dataframe = pd.DataFrame(columns=gl_df_columns)


def run_all_experiments(expname, name, indexes, labels, threads, dense=True, *args, **kwargs):
    # Don't move this file
    if not dense:
        name = f'{name}-sparse'
    else:
        name = f'{name}-dense'
    repo_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

    pibench_bin = os.path.join(
        repo_dir, 'build/_deps/pibench-build/src/PiBench')
    print('PiBench binary at:', pibench_bin)

    if not os.path.isfile(pibench_bin):
        print('PiBench binary not found')
        return

    if not os.access(pibench_bin, os.R_OK | os.W_OK | os.X_OK):
        print('PiBench binary permission denied')
        return

    PiBenchExperiment.pibench_bin = pibench_bin

    experiments = []

    btree_indexes = [index for index in indexes if 'btree' in index]
    art_indexes = [index for index in indexes if 'art' in index]
    btree_labels = [label for label in labels if 'B+Tree' in label]
    art_labels = [label for label in labels if 'ART' in label]

    assert(len(indexes) == len(labels))

    estimated_sec = prod(
        [len(indexes), len(threads), NUM_REPLICATES, kwargs['seconds']])
    print('Estimated time:', estimated_sec //
          60, 'minutes (excluding load time)')

    try:
        os.mkdir(os.path.join(data_dir, f'{name}.raw'))
    except:
        print(f'Warning: directory "{name}.raw" already exists')

    for index in indexes:
        wrapper_bin = os.path.join(
            repo_dir, 'build/wrappers/lib{}_wrapper.so'.format(index))
        for t in threads:
            if t == 1 and 'otdl' in index:
                continue
            experiments.append(PiBenchExperiment(
                name, index, wrapper_bin, dense, mode='time', pcm=False,
                threads=t, **kwargs))

    for i in range(NUM_REPLICATES):
        for j, exp in enumerate(experiments):
            exp.run(i + 1, i * len(experiments) + j + 1, len(experiments)
                    * NUM_REPLICATES)

    df_columns = ['index', 'thread', 'replicate'] + PiBenchExperiment.finishTypes + [
        f'{opType}-{finishType}' for opType in PiBenchExperiment.opTypes for finishType in PiBenchExperiment.finishTypes]
    objs = []

    for exp, (index, t) in zip(experiments, [(index, t) for index in indexes for t in threads]):
        for rid, results in enumerate(exp.results):
            exp_result_digest = results.sum(axis=0).to_numpy().tolist()
            exp_result_raw = results.to_numpy().flatten().tolist()
            row = pd.DataFrame(
                [[index, t, rid+1, *exp_result_digest, *exp_result_raw]], columns=df_columns)
            objs.append(row)

    df = pd.concat(objs, ignore_index=True)
    df.to_csv(os.path.join(data_dir, f'{expname}-{name}.csv'))

    plotFinishType = 'succeeded'

    def rstddev(x):
        return np.std(x, ddof=1) / np.mean(x) * 100
    df_digest = df.groupby(['index', 'thread']).agg(
        ['mean', 'min', 'max', rstddev])[plotFinishType]
    print(df_digest)
    df_digest.to_csv(os.path.join(data_dir, f'{expname}-{name}-digest.csv'))

    insert_ratio = 0.0
    read_ratio = 1.0
    update_ratio = 0.0
    remove_ratio = 0.0
    scan_ratio = 0.0
    if 'insert_ratio' in kwargs:
        insert_ratio = kwargs['insert_ratio']
    if 'read_ratio' in kwargs:
        read_ratio = kwargs['read_ratio']
    if 'update_ratio' in kwargs:
        update_ratio = kwargs['update_ratio']
    if 'remove_ratio' in kwargs:
        remove_ratio = kwargs['remove_ratio']
    if 'scan_ratio' in kwargs:
        scan_ratio = kwargs['scan_ratio']

    df['exp'] = expname
    df['name'] = name
    df['Insert-ratio'] = insert_ratio
    df['Read-ratio'] = read_ratio
    df['Update-ratio'] = update_ratio
    df['Remove-ratio'] = remove_ratio
    df['Scan-ratio'] = scan_ratio

    if kwargs['distribution'] == 'UNIFORM':
        df['distribution'] = 'uniform'
        df['skew-factor'] = 0
    elif kwargs['distribution'] == 'SELFSIMILAR':
        df['distribution'] = 'selfsimilar'
        df['skew-factor'] = kwargs['skew']
    else:
        raise ValueError
    if dense:
        df['key-type'] = 'dense-int'
    else:
        df['key-type'] = 'sparse-int'

    df.reindex(columns=gl_df_columns)
    global dataframe
    dataframe = dataframe._append(df, ignore_index=True)


if __name__ == '__main__': 
    index_bases = [
        #'btreeomcs_leaf_noop',
        'btreeomcs_leaf_op_read',
        'btreelc_stdrw',
        #'btreeotcl_noop_numa',
        #'btreeotcl_numa',
        #'btreeotcl_mutex_spin_then_park_numa',
        'btreeofp_noop_numa',
        'btreeofp_numa',
        #'btreeoaqs_numa',
        #'btreeoqspinlock_numa',
        #'btreeofp_mutex_spin_then_park_numa',
        #'btreeofp_mutex_optimized_numa',

        #'artomcs_noop',
        'artomcs_op_read',
        'artlc_stdrw',
        #'arttcl_numa',
        #'arttcl_mutex_spin_then_park_numa',
        #'arttcl_noop_numa',
        'artofp_numa',
        'artofp_noop_numa',
        #'artaqs_numa',
        #'artqspinlock_numa',
        # 'bwtree',
        #'artofp_mutex_spin_then_park_numa',
        #'artofp_mutex_optimized_numa',
    ]
    label_bases = [
        #'B+-tree OptiQL-NOR',
        'B+-tree OptiQL',
        'B+-tree STDRW',
        #'B+-tree OpTCL-NOR',
        #'B+-tree OpTCL',
        #'B+-tree OpTCL-Mutex',
        'B+-tree OpFP-NOR',
        'B+-tree OpFP',
        #'B+-tree OpAQS',
        #'B+-tree OpQSPINLOCK',
        #'B+-tree OpFP-Mutex',
        #'B+-tree OpFP-Mutex-Optimized',

        #'ART OptiQL-NOR',
        'ART OptiQL',
        'ART STDRW',
        #'ART OpTCL',
        #'ART OpTCL-Mutex',
        #'ART OpTCL-NOR',
        'ART OpFP',
        'ART OpFP-NOR',
        #'ART OpAQS',
        #'ART OpQspinlock',
        # 'Bw-Tree',
        #'ART OpFP-Mutex',
        #'ART OpFP-Mutex-Optimized',
    ]

    page_size_suffices = [''] #, '_4K', '_16K']
    indexes = [
        index + suffix for index in index_bases for suffix in page_size_suffices]
    labels = [
        label + suffix for label in label_bases for suffix in page_size_suffices]


    for num_record,record_label in zip(NUM_RECORDS, NUM_RECORD_LABELS): 
        for skew in SKEW:
            data_dir = base_data_dir + "-" + record_label + "-" + SERVER + "-skew-" + str(skew)
            try:
                os.makedirs(data_dir)
            except:
                print(f'Raw data directory already exists')
            # dense
            run_all_experiments('scalability', 'Read-only-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=1.0, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Write-heavy-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.2, update_ratio=0.8, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Read-heavy-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.8, update_ratio=0.2, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Balanced-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.5, update_ratio=0.5, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Update-only-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.0, update_ratio=1.0, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Read-heavy-Insert-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.8, insert_ratio=0.2, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Balanced-Insert-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.5, insert_ratio=0.5, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Insert-only-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0.0, insert_ratio=1.0, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Update-heavy-Insert-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0, insert_ratio=0.2, update_ratio=0.8, distribution='SELFSIMILAR', skew=skew)

            run_all_experiments('scalability', 'Balanced-Insert-Update-selfsimilar', indexes, labels, threads, records=num_record, seconds=SECONDS,
                                read_ratio=0, insert_ratio=0.5, update_ratio=0.5, distribution='SELFSIMILAR', skew=skew)

    dataframe.to_csv(os.path.join(data_dir, 'All.csv'))

    print("finished mixed benchmarks, exiting for now")
    exit(0)
