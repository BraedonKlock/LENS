import React, { useEffect, useRef } from 'react';
import { View, TouchableOpacity, Animated, StyleSheet, Text } from 'react-native';
import { Ionicons } from '@expo/vector-icons';
import { useNavigation } from '@react-navigation/native';
import { useNotifications } from '../context/NotificationContext';

export default function NotificationBell() {
  const { unreadCount } = useNotifications();
  const navigation = useNavigation();
  const rotateAnim = useRef(new Animated.Value(0)).current;
  const scaleAnim = useRef(new Animated.Value(1)).current;
  const glowAnim = useRef(new Animated.Value(0)).current;
  const prevCount = useRef(unreadCount);

  useEffect(() => {
    if (unreadCount > prevCount.current) {
      // Shake + pulse when a new notification arrives
      Animated.sequence([
        Animated.timing(rotateAnim, { toValue: 1, duration: 80, useNativeDriver: true }),
        Animated.timing(rotateAnim, { toValue: -1, duration: 80, useNativeDriver: true }),
        Animated.timing(rotateAnim, { toValue: 0.7, duration: 80, useNativeDriver: true }),
        Animated.timing(rotateAnim, { toValue: -0.7, duration: 80, useNativeDriver: true }),
        Animated.timing(rotateAnim, { toValue: 0, duration: 80, useNativeDriver: true }),
      ]).start();
      Animated.sequence([
        Animated.timing(scaleAnim, { toValue: 1.35, duration: 120, useNativeDriver: true }),
        Animated.timing(scaleAnim, { toValue: 1, duration: 120, useNativeDriver: true }),
      ]).start();
    }
    prevCount.current = unreadCount;
  }, [unreadCount]);

  // Continuous glow pulse when there are unread notifications
  useEffect(() => {
    if (unreadCount > 0) {
      const pulse = Animated.loop(
        Animated.sequence([
          Animated.timing(glowAnim, { toValue: 1, duration: 900, useNativeDriver: true }),
          Animated.timing(glowAnim, { toValue: 0, duration: 900, useNativeDriver: true }),
        ])
      );
      pulse.start();
      return () => pulse.stop();
    } else {
      glowAnim.setValue(0);
    }
  }, [unreadCount > 0]);

  const rotation = rotateAnim.interpolate({
    inputRange: [-1, 0, 1],
    outputRange: ['-30deg', '0deg', '30deg'],
  });

  const glowOpacity = glowAnim.interpolate({
    inputRange: [0, 1],
    outputRange: [0.2, 0.9],
  });

  return (
    <TouchableOpacity onPress={() => navigation.navigate('Incidents')} style={styles.container}>
      {/* Glow ring behind bell */}
      {unreadCount > 0 && (
        <Animated.View style={[styles.glow, { opacity: glowOpacity }]} />
      )}

      <Animated.View style={{ transform: [{ rotate: rotation }, { scale: scaleAnim }] }}>
        <Ionicons
          name={unreadCount > 0 ? 'notifications' : 'notifications-outline'}
          size={26}
          color={unreadCount > 0 ? '#FF4444' : '#8B949E'}
        />
      </Animated.View>

      {unreadCount > 0 && (
        <View style={styles.badge}>
          <Text style={styles.badgeText}>{unreadCount > 9 ? '9+' : unreadCount}</Text>
        </View>
      )}
    </TouchableOpacity>
  );
}

const styles = StyleSheet.create({
  container: {
    position: 'relative',
    width: 44,
    height: 44,
    alignItems: 'center',
    justifyContent: 'center',
  },
  glow: {
    position: 'absolute',
    width: 38,
    height: 38,
    borderRadius: 19,
    backgroundColor: '#FF4444',
  },
  badge: {
    position: 'absolute',
    top: 4,
    right: 4,
    backgroundColor: '#FF4444',
    borderRadius: 8,
    minWidth: 17,
    height: 17,
    alignItems: 'center',
    justifyContent: 'center',
    paddingHorizontal: 3,
    borderWidth: 1.5,
    borderColor: '#0D1117',
  },
  badgeText: {
    color: 'white',
    fontSize: 10,
    fontWeight: 'bold',
    lineHeight: 12,
  },
});
