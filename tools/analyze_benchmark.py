#!/usr/bin/env python3
"""
Benchmark analyzer for Perfect Dark Remaster recompilation.

Usage:
    python analyze_benchmark.py --file benchmark.csv
    python analyze_benchmark.py --file benchmark.csv --percentile 95
"""

import argparse
import csv
import sys
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class BenchmarkStats:
    """Statistics for benchmark data."""
    total_frames: int
    valid_frames: int
    avg_fps: float
    min_fps: int
    max_fps: int
    avg_frame_time_us: float
    min_frame_time_us: int
    max_frame_time_us: int
    p50_fps: float
    p95_fps: float
    p99_fps: float
    p99_frame_time_us: float
    avg_draw_calls: float
    avg_vertices: float
    avg_functions_dispatched: float
    avg_active_threads: float
    frame_time_stddev: float
    fps_stddev: float


def percentile(values: List[int], p: float) -> float:
    """Calculate percentile using linear interpolation."""
    if not values:
        return 0.0
    sorted_values = sorted(values)
    k = (len(sorted_values) - 1) * (p / 100.0)
    f = int(k)
    c = min(f + 1, len(sorted_values) - 1)
    if f == c:
        return float(sorted_values[f])
    return sorted_values[f] * (c - k) + sorted_values[c] * (k - f)


def stddev(values: List[float]) -> float:
    """Calculate standard deviation."""
    if len(values) < 2:
        return 0.0
    mean = sum(values) / len(values)
    variance = sum((x - mean) ** 2 for x in values) / len(values)
    return variance ** 0.5


def analyze_benchmark(file_path: str, fps_min: int = 10, fps_max: int = 200) -> BenchmarkStats:
    """Analyze benchmark CSV file and return statistics."""
    frames = []
    fps_values = []
    draw_calls = []
    vertices = []
    functions_dispatched = []
    active_threads = []

    with open(file_path, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                frame_time = int(row['frame_time_us'])
                fps = int(row['fps'])

                # Skip initialization frames and outliers
                if fps < fps_min or fps > fps_max:
                    continue

                frames.append(frame_time)
                fps_values.append(fps)
                draw_calls.append(int(row['draw_calls']))
                vertices.append(int(row['vertices_processed']))
                functions_dispatched.append(int(row['functions_dispatched']))
                active_threads.append(int(row['active_threads']))
            except (ValueError, KeyError):
                continue

    if not frames:
        raise ValueError("No valid frames found in benchmark file")

    return BenchmarkStats(
        total_frames=len(frames),
        valid_frames=len(frames),
        avg_fps=sum(fps_values) / len(fps_values),
        min_fps=min(fps_values),
        max_fps=max(fps_values),
        avg_frame_time_us=sum(frames) / len(frames),
        min_frame_time_us=min(frames),
        max_frame_time_us=max(frames),
        p50_fps=percentile(fps_values, 50),
        p95_fps=percentile(fps_values, 95),
        p99_fps=percentile(fps_values, 99),
        p99_frame_time_us=percentile(frames, 99),
        avg_draw_calls=sum(draw_calls) / len(draw_calls),
        avg_vertices=sum(vertices) / len(vertices),
        avg_functions_dispatched=sum(functions_dispatched) / len(functions_dispatched),
        avg_active_threads=sum(active_threads) / len(active_threads),
        frame_time_stddev=stddev([float(x) for x in frames]),
        fps_stddev=stddev([float(x) for x in fps_values]),
    )


def print_stats(stats: BenchmarkStats):
    """Print formatted statistics."""
    print("=" * 60)
    print("BENCHMARK ANALYSIS")
    print("=" * 60)
    print(f"Total frames analyzed:  {stats.total_frames}")
    print(f"Valid frames:           {stats.valid_frames}")
    print("-" * 60)
    print("FRAME TIME")
    print("-" * 60)
    print(f"Average:                {stats.avg_frame_time_us:.0f} us ({stats.avg_frame_time_us / 1000:.2f} ms)")
    print(f"Minimum:                {stats.min_frame_time_us} us")
    print(f"Maximum:                {stats.max_frame_time_us} us")
    print(f"P50:                    {stats.p50_fps:.1f} FPS")
    print(f"P95:                    {stats.p95_fps:.1f} FPS")
    print(f"P99:                    {stats.p99_fps:.1f} FPS")
    print(f"P99 frame time:         {stats.p99_frame_time_us:.0f} us")
    print(f"Stddev:                 {stats.frame_time_stddev:.0f} us")
    print("-" * 60)
    print("FPS")
    print("-" * 60)
    print(f"Average:                {stats.avg_fps:.2f}")
    print(f"Minimum:                {stats.min_fps}")
    print(f"Maximum:                {stats.max_fps}")
    print(f"Stddev:                 {stats.fps_stddev:.2f}")
    print("-" * 60)
    print("GPU/CPU WORKLOAD")
    print("-" * 60)
    print(f"Average draw calls:     {stats.avg_draw_calls:.1f}")
    print(f"Average vertices:       {stats.avg_vertices:.0f}")
    print(f"Average functions:      {stats.avg_functions_dispatched:.0f}")
    print(f"Average active threads: {stats.avg_active_threads:.1f}")
    print("=" * 60)


def main():
    parser = argparse.ArgumentParser(
        description="Analyze benchmark CSV file",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        '--file',
        required=True,
        help='Path to benchmark CSV file'
    )
    parser.add_argument(
        '--fps-min',
        type=int,
        default=10,
        help='Minimum FPS to consider valid (default: 10)'
    )
    parser.add_argument(
        '--fps-max',
        type=int,
        default=200,
        help='Maximum FPS to consider valid (default: 200)'
    )
    parser.add_argument(
        '--json',
        action='store_true',
        help='Output as JSON'
    )

    args = parser.parse_args()

    try:
        stats = analyze_benchmark(args.file, args.fps_min, args.fps_max)

        if args.json:
            import json
            print(json.dumps(stats.__dict__, indent=2))
        else:
            print_stats(stats)

    except FileNotFoundError:
        print(f"Error: File not found: {args.file}", file=sys.stderr)
        sys.exit(1)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
