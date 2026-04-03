#!/usr/bin/env python3
"""
AI Optimization Ranker for Compiler
Uses machine learning to predict optimal optimization passes
"""

import numpy as np
import torch
import torch.nn as nn
from typing import List, Dict, Tuple
from dataclasses import dataclass
import json
import pickle
from pathlib import Path

@dataclass
class OptimizationConfig:
    """Configuration for optimization passes"""
    name: str
    parameters: Dict[str, float]
    expected_benefit: float

class IREncoder(nn.Module):
    """Neural network encoder for IR features"""
    def __init__(self, input_dim: int = 15, hidden_dim: int = 64):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, 32)
        )
        
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.encoder(x)

class OptimizationRanker(nn.Module):
    """Main model for ranking optimization passes"""
    def __init__(self, num_passes: int = 10):
        super().__init__()
        self.encoder = IREncoder()
        self.ranker = nn.Sequential(
            nn.Linear(32, 64),
            nn.ReLU(),
            nn.Linear(64, num_passes),
            nn.Sigmoid()
        )
        
        # Pass names
        self.pass_names = [
            "ConstantFolding",
            "DCE",
            "LICM",
            "Inlining",
            "StrengthReduction",
            "TailCallElim",
            "LoopUnroll",
            "Vectorization",
            "Mem2Reg",
            "SROA"
        ]
        
    def forward(self, features: torch.Tensor) -> torch.Tensor:
        encoded = self.encoder(features)
        scores = self.ranker(encoded)
        return scores
    
    def get_recommendations(self, features: np.ndarray) -> List[OptimizationConfig]:
        """Get ranked list of optimization recommendations"""
        with torch.no_grad():
            tensor = torch.FloatTensor(features).unsqueeze(0)
            scores = self.forward(tensor).squeeze().numpy()
        
        recommendations = []
        for i, (pass_name, score) in enumerate(zip(self.pass_names, scores)):
            if score > 0.3:  # Threshold
                config = OptimizationConfig(
                    name=pass_name,
                    parameters=self._get_optimal_params(pass_name, score),
                    expected_benefit=float(score)
                )
                recommendations.append(config)
        
        # Sort by expected benefit
        recommendations.sort(key=lambda x: x.expected_benefit, reverse=True)
        return recommendations
    
    def _get_optimal_params(self, pass_name: str, score: float) -> Dict[str, float]:
        """Get optimal parameters based on predicted benefit"""
        params = {}
        if pass_name == "Inlining":
            params["threshold"] = int(50 + score * 200)
        elif pass_name == "LoopUnroll":
            params["unroll_factor"] = int(2 + score * 6)
        elif pass_name == "Vectorization":
            params["vector_width"] = int(4 if score < 0.6 else 8)
        return params

class PatternLearner:
    """Learns optimization patterns from execution data"""
    def __init__(self):
        self.patterns = {}
        self.execution_history = []
        
    def record_execution(self, function_hash: str, features: np.ndarray, 
                        optimizations: List[str], execution_time: float):
        """Record execution result for learning"""
        self.execution_history.append({
            'function_hash': function_hash,
            'features': features,
            'optimizations': optimizations,
            'execution_time': execution_time
        })
        
    def learn_patterns(self) -> Dict[str, any]:
        """Analyze history to find successful patterns"""
        if len(self.execution_history) < 10:
            return {}
            
        # Simple pattern: which passes correlate with speedup
        pass_performance = {}
        for record in self.execution_history:
            for opt in record['optimizations']:
                if opt not in pass_performance:
                    pass_performance[opt] = []
                pass_performance[opt].append(record['execution_time'])
        
        # Calculate average performance per pass
        patterns = {}
        for opt, times in pass_performance.items():
            patterns[opt] = {
                'avg_time': np.mean(times),
                'count': len(times),
                'confidence': min(len(times) / 100, 1.0)
            }
            
        self.patterns = patterns
        return patterns
    
    def predict_benefit(self, features: np.ndarray, pass_name: str) -> float:
        """Predict benefit of applying a pass based on learned patterns"""
        if pass_name not in self.patterns:
            return 0.5  # Default uncertainty
        
        pattern = self.patterns[pass_name]
        # Simple heuristic: higher confidence + lower avg time = better
        benefit = pattern['confidence'] * (1.0 / (1.0 + pattern['avg_time'] / 1000))
        return min(benefit, 1.0)

class AIOptimizationServer:
    """Server to handle requests from C++ compiler"""
    def __init__(self, model_path: str = None):
        self.device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        self.ranker = OptimizationRanker().to(self.device)
        self.learner = PatternLearner()
        
        if model_path and Path(model_path).exists():
            self.load_model(model_path)
            
    def analyze_function(self, features_dict: Dict) -> str:
        """Analyze function and return recommendations as JSON"""
        features = np.array([
            features_dict.get('basicBlockCount', 0),
            features_dict.get('instructionCount', 0),
            features_dict.get('loopCount', 0),
            features_dict.get('callCount', 0),
            features_dict.get('memoryOpCount', 0),
            features_dict.get('branchCount', 0),
            features_dict.get('cyclomaticComplexity', 0),
            features_dict.get('averageBlockSize', 0),
            features_dict.get('instructionDensity', 0),
            float(features_dict.get('hasRecursion', False)),
            float(features_dict.get('hasIndirectCalls', False)),
            float(features_dict.get('hasExceptionHandling', False)),
            float(features_dict.get('hasVectorizableLoops', False)),
            float(features_dict.get('hasMemoryAliasing', False)),
            features_dict.get('previousExecutionTime', 0) / 1000.0  # Normalize
        ])
        
        recommendations = self.ranker.get_recommendations(features)
        
        result = {
            'recommendations': [
                {
                    'pass': rec.name,
                    'benefit': rec.expected_benefit,
                    'parameters': rec.parameters
                }
                for rec in recommendations
            ],
            'features': features_dict
        }
        
        return json.dumps(result)
    
    def record_feedback(self, function_hash: str, features_dict: Dict,
                       applied_passes: List[str], execution_time: float):
        """Record execution feedback for learning"""
        features = np.array(list(features_dict.values()))
        self.learner.record_execution(function_hash, features, applied_passes, execution_time)
        
    def save_model(self, path: str):
        """Save model to disk"""
        torch.save(self.ranker.state_dict(), path)
        with open(path + '.patterns', 'wb') as f:
            pickle.dump(self.learner.patterns, f)
            
    def load_model(self, path: str):
        """Load model from disk"""
        self.ranker.load_state_dict(torch.load(path, map_location=self.device))
        pattern_path = path + '.patterns'
        if Path(pattern_path).exists():
            with open(pattern_path, 'rb') as f:
                self.learner.patterns = pickle.load(f)

# Flask API for C++ integration
from flask import Flask, request, jsonify

app = Flask(__name__)
server = None

@app.route('/analyze', methods=['POST'])
def analyze():
    data = request.json
    result = server.analyze_function(data['features'])
    return result

@app.route('/feedback', methods=['POST'])
def feedback():
    data = request.json
    server.record_feedback(
        data['function_hash'],
        data['features'],
        data['passes'],
        data['execution_time']
    )
    return jsonify({'status': 'ok'})

@app.route('/save', methods=['POST'])
def save():
    path = request.json.get('path', 'model.pt')
    server.save_model(path)
    return jsonify({'status': 'saved'})

def start_server(model_path: str = None, port: int = 5000):
    global server
    server = AIOptimizationServer(model_path)
    app.run(host='0.0.0.0', port=port)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', help='Path to model file')
    parser.add_argument('--port', type=int, default=5000)
    args = parser.parse_args()
    
    start_server(args.model, args.port)
